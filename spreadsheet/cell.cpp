#include "cell.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>

Cell::Cell(const SheetInterface &sheet)
    : sheet_(sheet) {
    impl_ = std::make_unique<EmptyImpl>();
}

Cell::~Cell() {}

void Cell::Set(std::string text) {
    // Если текст начинается со знака "=",
    // то он интерпретируется как формула.
    if (text[0] == '=' && text.size() > 1) {
        impl_ = std::make_unique<FormulaImpl>(text.substr(1));
    }
    else {
        impl_ = std::make_unique<TextImpl>(text);
    }
}

void Cell::Clear() {
    impl_ = std::make_unique<EmptyImpl>();
}

Cell::Value Cell::GetValue() const {
    if (!cache_.has_value()) {
        cache_ = impl_->GetValue(sheet_);
    }
    return cache_.value();
}
std::string Cell::GetText() const {
    return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const {
    return impl_->GetReferencedCells();
}

const std::unordered_set<Position, Position::PositionHasher> &Cell::GetDependentCells() const
{
    return dependent_cells_;
}

void Cell::AddDependentCell(Position pos)
{
    dependent_cells_.insert(pos);
}

void Cell::RemoveDependentCell(Position pos)
{
    dependent_cells_.erase(pos);
}

void Cell::ClearCache()
{
    cache_.reset();
}
