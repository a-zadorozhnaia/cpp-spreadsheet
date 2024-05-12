#pragma once

#include "common.h"
#include "formula.h"

#include <functional>
#include <unordered_set>
#include <optional>

class Sheet;

class Cell : public CellInterface {
public:
    Cell(const SheetInterface& sheet);
    ~Cell();

    void Set(std::string text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;

    std::vector<Position> GetReferencedCells() const override;
    const std::unordered_set<Position, Position::PositionHasher>& GetDependentCells() const;

    void AddDependentCell(Position pos);
    void RemoveDependentCell(Position pos);

    void ClearCache();

private:
    class Impl {
    public:
        virtual ~Impl() = default;
        virtual Value GetValue(const SheetInterface& sheet) const = 0;
        virtual std::string GetText() const = 0;
        virtual std::vector<Position> GetReferencedCells() const = 0;
    };

    class EmptyImpl : public Impl {
    public:
        Value GetValue(const SheetInterface&) const override {return "";}
        std::string GetText() const override {return "";}
        std::vector<Position> GetReferencedCells() const override {return {};}
    };

    class TextImpl : public Impl {
    public:
        TextImpl(std::string s)
            : text_(std::move(s)) {
        }
        Value GetValue(const SheetInterface&) const override {
            if (text_[0] == '\'') {
                return text_.substr(1);
            }
            return text_;
        }
        std::string GetText() const override {
            return text_;
        }
        std::vector<Position> GetReferencedCells() const override {return {};}
    private:
        std::string text_;
    };

    class FormulaImpl : public Impl {
    public:
        FormulaImpl(std::string s)
            : formula_(ParseFormula(s)) {
        }
        Value GetValue(const SheetInterface& sheet) const override {
            auto value = formula_->Evaluate(sheet);
            if (std::holds_alternative<FormulaError>(value)) {
                return std::get<FormulaError>(value);
            }
            return std::get<double>(value);
        }
        std::string GetText() const override {
            return "=" + formula_->GetExpression();
        }
        std::vector<Position> GetReferencedCells() const override {
            return  formula_->GetReferencedCells();
        }
    private:
        std::unique_ptr<FormulaInterface> formula_;
    };

    std::unique_ptr<Impl> impl_;
    mutable std::optional<Value> cache_;
    const SheetInterface& sheet_;
    std::unordered_set<Position, Position::PositionHasher> dependent_cells_;
};
