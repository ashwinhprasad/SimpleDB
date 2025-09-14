#include "parser.hpp"
#include <memory>
#include <sstream>
#include <utility>

// ---- Small helpers to construct AST nodes ----------------------------------

static Expr make_ident(std::string s) {
    Expr e;
    e.node = std::move(s);
    return e;
}

static Expr make_literal(Value v) {
    Expr e;
    e.node = std::move(v);
    return e;
}

static Expr make_unary(Expr::Unary::Op op, Expr inner) {
    Expr e;
    Expr::Unary u;
    u.op = op;
    u.expr = std::make_unique<Expr>(std::move(inner));
    e.node = std::move(u);
    return e;
}

static Expr make_binary(Expr left, Expr::Binary::Op op, Expr right) {
    Expr e;
    Expr::Binary b;
    b.op = op;
    b.lhs = std::make_unique<Expr>(std::move(left));
    b.rhs = std::make_unique<Expr>(std::move(right));
    e.node = std::move(b);
    return e;
}

// ---- Token name helper (for clearer errors) --------------------------------

static const char* token_name(Token t) {
    switch (t) {
        case Token::LParen: return "LParen";
        case Token::RParen: return "RParen";
        case Token::Comma:  return "Comma";
        case Token::Plus:   return "Plus";
        case Token::Semi:   return "Semi";
        case Token::Star:   return "Star";
        case Token::Eq:     return "Eq";
        case Token::Neq:    return "Neq";
        case Token::Lt:     return "Lt";
        case Token::Lte:    return "Lte";
        case Token::Gt:     return "Gt";
        case Token::Gte:    return "Gte";
        case Token::Dot:    return "Dot";
        case Token::Ident:  return "Ident";
        case Token::String: return "String";
        case Token::Int:    return "Int";
        case Token::Float:  return "Float";
        case Token::KwCreate: return "CREATE";
        case Token::KwTable:  return "TABLE";
        case Token::KwDrop:   return "DROP";
        case Token::KwIf:     return "IF";
        case Token::KwExists: return "EXISTS";
        case Token::KwInsert: return "INSERT";
        case Token::KwInto:   return "INTO";
        case Token::KwValues: return "VALUES";
        case Token::KwDelete: return "DELETE";
        case Token::KwFrom:   return "FROM";
        case Token::KwWhere:  return "WHERE";
        case Token::KwUpdate: return "UPDATE";
        case Token::KwSet:    return "SET";
        case Token::KwSelect: return "SELECT";
        case Token::KwAnd:    return "AND";
        case Token::KwOr:     return "OR";
        case Token::KwNot:    return "NOT";
        case Token::KwNull:   return "NULL";
        case Token::KwTrue:   return "TRUE";
        case Token::KwFalse:  return "FALSE";
        case Token::KwLimit:  return "LIMIT";
        case Token::KwInt:    return "INT";
        case Token::KwInteger:return "INTEGER";
        case Token::KwText:   return "TEXT";
        case Token::KwReal:   return "REAL";
        case Token::KwFloat:  return "FLOAT";
        case Token::KwBool:   return "BOOL";
    }
    return "?";
}

// ---- Parser ---------------------------------------------------------------

class Parser {
public:
    explicit Parser(std::vector<TokenValue> toks) : tokens(std::move(toks)) {}

    Statement parse_statement() {
        const Token* k = peek_kind();
        if (!k) throw err("expected a statement, got <eof>");

        switch (*k) {
            case Token::KwCreate: return parse_create_table();
            case Token::KwDrop:   return parse_drop_table();
            case Token::KwInsert: return parse_insert();
            case Token::KwDelete: return parse_delete();
            case Token::KwUpdate: return parse_update();
            case Token::KwSelect: return parse_select();
            default:
                throw err("expected a statement, got " + got_here());
        }
    }

    bool eat(Token t) {
        if (!eof() && tokens[pos].kind == t) { pos++; return true; }
        return false;
    }

    void expect(Token t) {
        if (eof() || tokens[pos].kind != t) {
            std::ostringstream oss;
            oss << "expected " << token_name(t) << ", got " << got_here();
            throw err(oss.str());
        }
        pos++;
    }

    std::string expect_ident() {
        if (eof()) throw err("expected identifier, got <eof>");
        const auto& tv = tokens[pos++];
        if (tv.kind == Token::Ident) return tv.text;
        std::ostringstream oss;
        oss << "expected identifier, got " << token_name(tv.kind);
        throw err(oss.str());
    }

    bool eof() const { return pos >= tokens.size(); }

    const Token* peek_kind() const {
        if (eof()) return nullptr;
        return &tokens[pos].kind;
    }

    const TokenValue* peek() const {
        if (eof()) return nullptr;
        return &tokens[pos];
    }

    const TokenValue* next() {
        if (eof()) return nullptr;
        return &tokens[pos++];
    }

private:
    std::vector<TokenValue> tokens;
    size_t pos = 0;

    ParseError err(std::string msg) const { return ParseError(std::move(msg)); }

    std::string got_here() const {
        if (eof()) return std::string("<eof>");
        std::ostringstream oss;
        oss << token_name(tokens[pos].kind);
        return oss.str();
    }

    // ---- Statements ----

    // CREATE TABLE name (col type, ...)
    Statement parse_create_table() {
        expect(Token::KwCreate);
        expect(Token::KwTable);
        std::string name = expect_ident();
        expect(Token::LParen);

        std::vector<ColumnDef> cols;
        while (true) {
            ColumnDef cdef;
            cdef.name = expect_ident();
            cdef.data_type = parse_type();
            cols.push_back(std::move(cdef));

            if (eat(Token::Comma)) continue;
            expect(Token::RParen);
            break;
        }

        Statement s;
        s.kind = Statement::CreateTable;
        Statement::CreateTableData d{ std::move(name), std::move(cols) };
        s.data = std::move(d);
        return s;
    }

    DataType parse_type() {
        const TokenValue* tv = next();
        if (!tv) throw err("expected type, got <eof>");

        DataType dt{};
        switch (tv->kind) {
            case Token::KwInt:
            case Token::KwInteger:
                dt.kind = DataType::Int; break;
            case Token::KwReal:
            case Token::KwFloat:
                dt.kind = DataType::Real; break;
            case Token::KwText:
                dt.kind = DataType::Text; break;
            case Token::KwBool:
                dt.kind = DataType::Bool; break;
            case Token::Ident:
                dt.kind = DataType::Custom;
                dt.custom = tv->text;
                break;
            default: {
                std::ostringstream oss; oss << "expected type, got " << token_name(tv->kind);
                throw err(oss.str());
            }
        }
        return dt;
    }

    // DROP TABLE [IF EXISTS] name
    Statement parse_drop_table() {
        expect(Token::KwDrop);
        expect(Token::KwTable);
        bool if_exists = false;
        if (peek_kind() && *peek_kind() == Token::KwIf) {
            next(); // IF
            expect(Token::KwExists);
            if_exists = true;
        }
        std::string name = expect_ident();

        Statement s;
        s.kind = Statement::DropTable;
        Statement::DropTableData d{ std::move(name), if_exists };
        s.data = std::move(d);
        return s;
    }

    // INSERT INTO name [(col, ...)] VALUES (expr, ...)
    Statement parse_insert() {
        expect(Token::KwInsert);
        expect(Token::KwInto);
        std::string table = expect_ident();

        std::optional<std::vector<std::string>> columns;
        if (eat(Token::LParen)) {
            std::vector<std::string> cols;
            while (true) {
                cols.push_back(expect_ident());
                if (eat(Token::Comma)) continue;
                expect(Token::RParen);
                break;
            }
            columns = std::move(cols);
        }

        expect(Token::KwValues);
        expect(Token::LParen);
        std::vector<Expr> values;
        while (true) {
            values.push_back(parse_expr());
            if (eat(Token::Comma)) continue;
            expect(Token::RParen);
            break;
        }

        Statement s;
        s.kind = Statement::Insert;
        Statement::InsertData d{ std::move(table), std::move(columns), std::move(values) };
        s.data = std::move(d);
        return s;
    }

    // DELETE FROM name [WHERE expr]
    Statement parse_delete() {
        expect(Token::KwDelete);
        expect(Token::KwFrom);
        std::string table = expect_ident();
        std::optional<Expr> selection;
        if (peek_kind() && *peek_kind() == Token::KwWhere) {
            next(); // WHERE
            selection = parse_expr();
        }

        Statement s;
        s.kind = Statement::Delete;
        Statement::DeleteData d{ std::move(table), std::move(selection) };
        s.data = std::move(d);
        return s;
    }

    // UPDATE name SET col = expr [, col = expr]* [WHERE expr]
    Statement parse_update() {
        expect(Token::KwUpdate);
        std::string table = expect_ident();
        expect(Token::KwSet);

        std::vector<std::pair<std::string, Expr>> assigns;
        while (true) {
            std::string col = expect_ident();
            expect(Token::Eq);
            Expr rhs = parse_expr();
            assigns.emplace_back(std::move(col), std::move(rhs));
            if (eat(Token::Comma)) continue;
            break;
        }

        std::optional<Expr> selection;
        if (peek_kind() && *peek_kind() == Token::KwWhere) {
            next(); // WHERE
            selection = parse_expr();
        }

        Statement s;
        s.kind = Statement::Update;
        Statement::UpdateData d{ std::move(table), std::move(assigns), std::move(selection) };
        s.data = std::move(d);
        return s;
    }

    // SELECT ( * | col [, col]* ) FROM name [WHERE expr] [LIMIT n]
    Statement parse_select() {
        expect(Token::KwSelect);

        std::vector<SelectItem> items;
        if (eat(Token::Star)) {
            SelectItem it;
            it.kind = SelectItem::Wildcard;
            items.push_back(std::move(it));
        } else {
            while (true) {
                std::string ident = expect_ident();
                SelectItem it;
                it.kind = SelectItem::Column;
                it.column = std::move(ident);
                items.push_back(std::move(it));
                if (eat(Token::Comma)) continue;
                break;
            }
        }

        expect(Token::KwFrom);
        std::string table = expect_ident();

        std::optional<Expr> selection;
        if (peek_kind() && *peek_kind() == Token::KwWhere) {
            next();
            selection = parse_expr();
        }

        std::optional<unsigned long long> limit;
        if (peek_kind() && *peek_kind() == Token::KwLimit) {
            next(); // LIMIT
            const TokenValue* tv = next();
            if (!tv || tv->kind != Token::Int || tv->int_val < 0) {
                throw err("expected non-negative integer for LIMIT");
            }
            limit = static_cast<unsigned long long>(tv->int_val);
        }

        Statement s;
        s.kind = Statement::Select;
        Statement::SelectData d{ std::move(items), std::move(table), std::move(selection), std::move(limit) };
        s.data = std::move(d);
        return s;
    }

    // ---- Expressions (Pratt parser) ----------------------------------------

    Expr parse_expr() { return parse_or(); }

    Expr parse_or() {
        Expr node = parse_and();
        while (peek_kind() && *peek_kind() == Token::KwOr) {
            next(); // OR
            Expr rhs = parse_and();
            node = make_binary(std::move(node), Expr::Binary::Or, std::move(rhs));
        }
        return node;
    }

    Expr parse_and() {
        Expr node = parse_cmp();
        while (peek_kind() && *peek_kind() == Token::KwAnd) {
            next(); // AND
            Expr rhs = parse_cmp();
            node = make_binary(std::move(node), Expr::Binary::And, std::move(rhs));
        }
        return node;
    }

    Expr parse_cmp() {
        Expr node = parse_add();
        while (true) {
            const Token* k = peek_kind();
            Expr::Binary::Op op;
            if (!k) break;

            if (*k == Token::Eq) op = Expr::Binary::Eq;
            else if (*k == Token::Neq) op = Expr::Binary::Neq;
            else if (*k == Token::Lt) op = Expr::Binary::Lt;
            else if (*k == Token::Lte) op = Expr::Binary::Lte;
            else if (*k == Token::Gt) op = Expr::Binary::Gt;
            else if (*k == Token::Gte) op = Expr::Binary::Gte;
            else break;

            next(); // consume operator
            Expr rhs = parse_add();
            node = make_binary(std::move(node), op, std::move(rhs));
        }
        return node;
    }

    // Note: arithmetic +,-,*,/ intentionally omitted (like the Rust version).
    Expr parse_add() {
        // Would normally handle + and -, but we skip them to match the Rust code.
        return parse_mul();
    }

    Expr parse_mul() {
        // Would normally handle * and /, but we skip them to match the Rust code.
        return parse_unary();
    }

    Expr parse_unary() {
        const Token* k = peek_kind();
        if (k && *k == Token::KwNot) {
            next();
            return make_unary(Expr::Unary::Not, parse_unary());
        }
        // unary minus not supported; negative numbers expected as literals only
        return parse_primary();
    }

    Expr parse_primary() {
        const TokenValue* tv = next();
        if (!tv) throw err("unexpected <eof> in expression");

        switch (tv->kind) {
            case Token::LParen: {
                Expr e = parse_expr();
                expect(Token::RParen);
                return e;
            }
            case Token::Ident:
                return make_ident(tv->text);
            case Token::String: {
                Value v; v.kind = Value::String; v.s = tv->text;
                return make_literal(std::move(v));
            }
            case Token::Int: {
                Value v; v.kind = Value::Int; v.i = tv->int_val;
                return make_literal(std::move(v));
            }
            case Token::Float: {
                Value v; v.kind = Value::Float; v.f = tv->float_val;
                return make_literal(std::move(v));
            }
            case Token::KwNull: {
                Value v; v.kind = Value::Null;
                return make_literal(std::move(v));
            }
            case Token::KwTrue: {
                Value v; v.kind = Value::Bool; v.b = true;
                return make_literal(std::move(v));
            }
            case Token::KwFalse: {
                Value v; v.kind = Value::Bool; v.b = false;
                return make_literal(std::move(v));
            }
            default: {
                std::ostringstream oss;
                oss << "unexpected token in expression: " << token_name(tv->kind);
                throw err(oss.str());
            }
        }
    }
};

// ---- Top-level parse() -----------------------------------------------------

Statement parse(const std::string &input) {
    auto toks = lex(input);
    Parser p(std::move(toks));
    Statement stmt = p.parse_statement();

    // trailing semicolons optional; ignore extra semicolons
    while (true) {
        const Token* k = p.peek_kind();
        if (k && *k == Token::Semi) { p.next(); continue; }
        break;
    }

    if (!p.eof()) {
        throw ParseError("unexpected tokens after statement");
    }
    return stmt;
}
