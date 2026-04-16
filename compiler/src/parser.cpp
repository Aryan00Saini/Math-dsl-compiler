#include "parser.h"
#include <iostream>
#include <stdexcept>

// ── Dummy token ───────────────────────────────────────────────────────────────
static Token dummyToken() {
  return Token(TokenType::EOF_TOKEN, "", 0, 0);
}

Parser::Parser(Lexer& lexer)
    : lexer_(lexer), current_(dummyToken()), previous_(dummyToken()) {
  advance();  // prime: current_ = first real token
}

// ── Error helpers ─────────────────────────────────────────────────────────────

void Parser::errorAt(const Token& token, const std::string& message) {
  if (panicMode_) return;
  panicMode_ = true;

  std::string where;
  if (token.type == TokenType::EOF_TOKEN)
    where = " at end";
  else if (token.type != TokenType::ERROR)
    where = " at '" + token.value + "'";

  std::cerr << "[line " << token.line << "] Error" << where
            << ": " << message << "\n";

  errors_.push_back({message, token.line, token.column});
}

void Parser::error(const std::string& message)           { errorAt(previous_, message); }
void Parser::errorAtCurrent(const std::string& message)  { errorAt(current_,  message); }

// ── Token helpers ─────────────────────────────────────────────────────────────

void Parser::advance() {
  previous_ = current_;
  for (;;) {
    current_ = lexer_.nextToken();
    if (current_.type != TokenType::ERROR) break;
    errorAtCurrent(current_.value);
  }
}

void Parser::consume(TokenType type, const std::string& message) {
  if (current_.type == type) { advance(); return; }
  errorAtCurrent(message);
}

bool Parser::check(TokenType type) const { return current_.type == type; }

bool Parser::match(TokenType type) {
  if (!check(type)) return false;
  advance();
  return true;
}

void Parser::synchronize() {
  panicMode_ = false;
  while (current_.type != TokenType::EOF_TOKEN) {
    if (previous_.type == TokenType::SEMICOLON) return;
    if (previous_.type == TokenType::RIGHT_BRACE) return;
    switch (current_.type) {
      case TokenType::LET:
      case TokenType::FN:
      case TokenType::IF:
      case TokenType::WHILE:
      case TokenType::RETURN:
      case TokenType::LEFT_BRACE:   // bare block is a valid statement start
        return;
      default: break;
    }
    advance();
  }
}

// ── Statements ────────────────────────────────────────────────────────────────

// statement → ifStmt | whileStmt | block | letDecl | exprStmt
ASTNodePtr Parser::statement() {
  if (match(TokenType::IF))         return ifStatement();
  if (match(TokenType::WHILE))      return whileStatement();
  if (match(TokenType::LET))        return letDeclaration();
  if (check(TokenType::LEFT_BRACE)) return block();  // bare scope block
  return exprStatement();
}

// ifStmt → "if" "(" equality ")" block ( "else" ( ifStmt | block ) )?
ASTNodePtr Parser::ifStatement() {
  consume(TokenType::LEFT_PAREN, "Expect '(' after 'if'.");
  ASTNodePtr cond = equality();
  consume(TokenType::RIGHT_PAREN, "Expect ')' after if condition.");

  ASTNodePtr thenBranch = block();
  std::optional<ASTNodePtr> elseBranch;

  if (match(TokenType::ELSE)) {
    if (check(TokenType::IF)) {
      advance();  // consume the 'if' token
      elseBranch = ifStatement();
    } else {
      elseBranch = block();
    }
  }

  return makeIf(std::move(cond), std::move(thenBranch), std::move(elseBranch));
}

// whileStmt → "while" "(" equality ")" block
ASTNodePtr Parser::whileStatement() {
  consume(TokenType::LEFT_PAREN, "Expect '(' after 'while'.");
  ASTNodePtr cond = equality();
  consume(TokenType::RIGHT_PAREN, "Expect ')' after while condition.");
  return makeWhile(std::move(cond), block());
}

// block → "{" statement* "}"
ASTNodePtr Parser::block() {
  consume(TokenType::LEFT_BRACE, "Expect '{'.");
  ASTNodePtr blk = makeBlock();

  while (!check(TokenType::RIGHT_BRACE) && !check(TokenType::EOF_TOKEN)) {
    ASTNodePtr stmt = statement();
    if (stmt)
      blockAddStatement(*blk, std::move(stmt));
    if (hadError()) synchronize();
  }

  consume(TokenType::RIGHT_BRACE, "Expect '}'.");
  return blk;
}

// letDecl → "let" IDENTIFIER "=" equality ";"
ASTNodePtr Parser::letDeclaration() {
  consume(TokenType::IDENTIFIER, "Expect variable name after 'let'.");
  std::string name = previous_.value;
  int declLine     = previous_.line;

  consume(TokenType::EQUALS, "Expect '=' after variable name.");
  ASTNodePtr value = equality();
  consume(TokenType::SEMICOLON, "Expect ';' after declaration.");

  return makeAssignment(std::move(name), std::move(value), /*isDecl=*/true, declLine);
}

// exprStmt → equality ";"
//          | IDENTIFIER "=" equality ";"     (bare assignment without 'let')
ASTNodePtr Parser::exprStatement() {
  // Peek ahead: is this a bare re-assignment?  IDENTIFIER "=" ...
  if (check(TokenType::IDENTIFIER)) {
    Token saved = current_;
    Token next  = lexer_.peekToken();
    if (next.type == TokenType::EQUALS) {
      advance();  // consume IDENTIFIER
      std::string name = previous_.value;
      int line         = previous_.line;
      advance();  // consume '='
      ASTNodePtr value = equality();
      consume(TokenType::SEMICOLON, "Expect ';' after assignment.");
      return makeAssignment(std::move(name), std::move(value), /*isDecl=*/false, line);
    }
    // else fall through to expression
    (void)saved;
  }

  ASTNodePtr expr = equality();
  consume(TokenType::SEMICOLON, "Expect ';' after expression.");
  return expr;
}

// ── Expressions ───────────────────────────────────────────────────────────────

// assignment → equality   (bare assignment already handled in exprStatement)
ASTNodePtr Parser::assignment() {
  return equality();
}

// equality → comparison ( ("==" | "!=") comparison )*
ASTNodePtr Parser::equality() {
  ASTNodePtr node = comparison();

  while (match(TokenType::EQUAL_EQUAL) || match(TokenType::BANG_EQUAL)) {
    TokenType  op    = previous_.type;
    ASTNodePtr right = comparison();
    node = makeBinary(op, std::move(node), std::move(right));
  }

  return node;
}

// comparison → expression ( ("<" | ">" | "<=" | ">=") expression )*
ASTNodePtr Parser::comparison() {
  ASTNodePtr node = expression();

  while (match(TokenType::LESS)    || match(TokenType::LESS_EQUAL) ||
         match(TokenType::GREATER) || match(TokenType::GREATER_EQUAL)) {
    TokenType  op    = previous_.type;
    ASTNodePtr right = expression();
    node = makeBinary(op, std::move(node), std::move(right));
  }

  return node;
}

// expression → term ( ("+" | "-") term )*
ASTNodePtr Parser::expression() {
  ASTNodePtr node = term();

  while (match(TokenType::PLUS) || match(TokenType::MINUS)) {
    TokenType  op    = previous_.type;
    ASTNodePtr right = term();
    node = makeBinary(op, std::move(node), std::move(right));
  }

  return node;
}

// term → power ( ("*" | "/") power )*
ASTNodePtr Parser::term() {
  ASTNodePtr node = power();

  while (match(TokenType::STAR) || match(TokenType::SLASH)) {
    TokenType  op    = previous_.type;
    ASTNodePtr right = power();
    node = makeBinary(op, std::move(node), std::move(right));
  }

  return node;
}

// power → unary ("^" power)?   ← single recursive call = right-associative
ASTNodePtr Parser::power() {
  ASTNodePtr base = unary();

  if (match(TokenType::CARET)) {
    ASTNodePtr exp = power();  // right-recursive
    return makeBinary(TokenType::CARET, std::move(base), std::move(exp));
  }

  return base;
}

// unary → ("!" | "-") unary | call
ASTNodePtr Parser::unary() {
  if (match(TokenType::BANG) || match(TokenType::MINUS)) {
    TokenType  op    = previous_.type;
    ASTNodePtr right = unary();
    return makeUnary(op, std::move(right));
  }
  return call();
}

// call → primary ( "(" args? ")" )?
ASTNodePtr Parser::call() {
  return primary();   // function calls resolved inside primary()
}

// primary → NUMBER | "true" | "false" | IDENTIFIER | IDENTIFIER "(" args ")" | "(" equality ")"
ASTNodePtr Parser::primary() {
  if (match(TokenType::NUMBER)) {
    double val = std::stod(previous_.value);
    return makeNumber(val);
  }

  if (match(TokenType::TRUE_KW))  return makeNumber(1.0);
  if (match(TokenType::FALSE_KW)) return makeNumber(0.0);

  if (match(TokenType::IDENTIFIER)) {
    std::string name = previous_.value;
    int         line = previous_.line;
    int         col  = previous_.column;

    // Function call?
    if (match(TokenType::LEFT_PAREN)) {
      std::vector<ASTNodePtr> args;
      if (!check(TokenType::RIGHT_PAREN)) {
        do {
          args.push_back(equality());
        } while (match(TokenType::COMMA));
      }
      consume(TokenType::RIGHT_PAREN, "Expect ')' after arguments.");
      return makeCall(std::move(name), std::move(args), line);
    }

    return makeVariable(std::move(name), line, col);
  }

  if (match(TokenType::LEFT_PAREN)) {
    ASTNodePtr node = equality();
    consume(TokenType::RIGHT_PAREN, "Expect ')' after expression.");
    return node;
  }

  error("Expect expression.");
  return nullptr;
}

// ── Entry point ───────────────────────────────────────────────────────────────

ASTNodePtr Parser::parse() {
  ASTNodePtr program = makeProgram();

  while (!check(TokenType::EOF_TOKEN)) {
    ASTNodePtr stmt = statement();
    if (stmt)
      programAddStatement(*program, std::move(stmt));
    if (hadError()) {
      synchronize();
      // Reset panic so we can parse further statements
      panicMode_ = false;
    }
  }

  auto& prog = std::get<ProgramNode>(program->data);
  if (prog.statements.empty()) return nullptr;

  return program;
}
