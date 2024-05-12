#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>
#include <functional>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << "#ARITHM!";
}

std::ostream& operator<<(std::ostream& output, FormulaException fe) {
    return output << std::string(fe.what());
}

FormulaError::FormulaError(Category category)
    : category_(category ) {}

FormulaError::Category FormulaError::GetCategory() const {
    return category_;
}

bool FormulaError::operator==(FormulaError rhs) const {
    return category_ == rhs.category_;
}

std::string_view FormulaError::ToString() const {
    using namespace std::literals;
    return "FormulaError"s;
}

namespace {
class Formula : public FormulaInterface {
public:
    explicit Formula(std::string expression) try
        : ast_(ParseFormulaAST(expression)) {

    } catch (const std::exception& exc) {
        std::throw_with_nested(FormulaException(exc.what()));
    }

    FormulaInterface::Value Evaluate(const SheetInterface& sheet) const override {
        std::function<double(Position)> get_val_by_pos = [&sheet](Position pos){
            if (!pos.IsValid()) {
                std::throw_with_nested(InvalidPositionException("Invalid position"s));
            }

            auto cell = sheet.GetCell(pos);
            if (!cell) {
                return 0.0;
            }
            auto res = cell->GetValue();
            if (std::holds_alternative<std::string>(res)) {
                auto str = std::get<std::string>(res);
                if (str == "") {
                    return 0.0;
                }
                if (str.find_first_not_of("+-0123456789") != std::string::npos) {
                    std::throw_with_nested(FormulaError(FormulaError::Category::Value));
                }
                return std::stod(str);
            } else if (std::holds_alternative<FormulaError>(res)) {
                std::throw_with_nested(std::get<FormulaError>(res));
            }
            return std::get<double>(res);
        };

        try {
            double result = ast_.Execute(std::ref(get_val_by_pos));
            return result;
        }  catch (const FormulaError& fe) {
            return fe;
        }
    }
    std::string GetExpression() const override {
        std::stringstream os;
        ast_.PrintFormula(os);
        return os.str();
    }

    std::vector<Position> GetReferencedCells() const override {
        std::vector<Position> cells(ast_.GetCells().begin(), ast_.GetCells().end());
        auto last = std::unique(cells.begin(), cells.end());
        cells.erase(last, cells.end());
        return cells;
    }

private:
    FormulaAST ast_;
};
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(std::move(expression));
}
