#pragma once
#include <string>
#include <vector>
#include <variant>
#include <optional>
#include <utility>
#include <memory>

struct ColumnDef;

struct Value {
    enum Kind { Null, Bool, Int, Float, String } kind;
    bool b{};
    long long i{};
    double f{};
    std::string s;
};

struct Expr {
    struct Unary { enum Op { Not, Neg }; Op op; std::unique_ptr<Expr> expr; };
    struct Binary {
        enum Op { Or, And, Eq, Neq, Lt, Lte, Gt, Gte, Add, Sub, Mul, Div };
        std::unique_ptr<Expr> lhs, rhs;
        Op op;
    };

    std::variant<Value, std::string, Unary, Binary> node;
};

struct Statement {
    
    enum Kind { CreateTable, DropTable, Insert, Delete, Update, Select } kind;

    struct CreateTableData {
        std::string name;
        std::vector<ColumnDef> columns;
    };
    struct DropTableData {
        std::string name;
        bool if_exists;
    };
    struct InsertData {
        std::string table;
        std::optional<std::vector<std::string>> columns;
        std::vector<struct Expr> values;
    };
    struct DeleteData {
        std::string table;
        std::optional<struct Expr> selection;
    };
    struct UpdateData {
        std::string table;
        std::vector<std::pair<std::string, struct Expr>> assignments;
        std::optional<struct Expr> selection;
    };
    struct SelectData {
        std::vector<struct SelectItem> columns;
        std::string table;
        std::optional<struct Expr> selection;
        std::optional<unsigned long long> limit;
    };

    std::variant<CreateTableData, DropTableData, InsertData, DeleteData, UpdateData, SelectData> data;
};

struct DataType {
    enum Kind { Int, Real, Text, Bool, Custom } kind;
    std::string custom;
};

struct ColumnDef {
    std::string name;
    DataType data_type;
};

struct SelectItem {
    enum Kind { Wildcard, Column } kind;
    std::string column;
};

