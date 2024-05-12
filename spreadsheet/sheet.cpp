#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

Sheet::~Sheet() {}

void Sheet::SetCell(Position pos, std::string text) {
    if (!pos.IsValid()) {
        std::throw_with_nested(InvalidPositionException("Invalid position"s));
    }

    // Создание ячейки
    Cell* old_cell = GetConcreteCell(pos);
    auto new_cell = std::make_unique<Cell>(*this);
    new_cell->Set(text);

    // Проверка наличия циклических зависимостей
    CheckCircularDependencies(pos, new_cell.get());
    // Если ячейка с таким индексом уже существовала
    if (old_cell) {
        // Удаление старой ячейки из обратных зависимостей ячеек, на которые она ссылалась
        RemoveDependentsCells(pos, old_cell->GetReferencedCells());
        // Копирование обратных зависимостей старой ячейки в новую
        CopyDependentsCells(new_cell.get(), old_cell->GetDependentCells());
        // Инвалидация кэша ячеек в обратных связях изменяемой ячейки
        InvalidateCache(pos);
    }
    // Добавление новой ячейки в обратные зависимости ячеек, на которые она ссылалается
    AddDependentsCells(pos, new_cell->GetReferencedCells());

    // Если "размер" таблицы меньше, чем задаваемая позиция, то увеличиваем размер таблицы (по строкам и/или столбцам)
    if (pos.row >= static_cast<int>(sheet_.size())) {
        sheet_.resize(pos.row + 1);
    }
    if (pos.col >= static_cast<int>(sheet_[pos.row].size())) {
        sheet_[pos.row].resize(pos.col + 1);
    }

    // Перемещение ячейки в таблицу
    sheet_[pos.row][pos.col] = std::move(new_cell);

    // Обновление размера
    if (pos.row >= printable_size_.rows) {
        printable_size_.rows = pos.row + 1;
    }
    if (pos.col >= printable_size_.cols) {
        printable_size_.cols = pos.col + 1;
    }
}

const CellInterface* Sheet::GetCell(Position pos) const {
    if (!pos.IsValid()) {
        std::throw_with_nested(InvalidPositionException("Invalid position"s));
    }

    // Если ячейки с заданным индексом не существует, возвращаем nullptr
    if (pos.row >= static_cast<int>(sheet_.size())) {
        return nullptr;
    }
    if (pos.col >= static_cast<int>(sheet_[pos.row].size())) {
        return nullptr;
    }

    return sheet_[pos.row][pos.col].get();
}

CellInterface* Sheet::GetCell(Position pos) {
    if (!pos.IsValid()) {
        std::throw_with_nested(InvalidPositionException("Invalid position"s));
    }

    // Если ячейки с заданным индексом не существует, возвращаем nullptr
    if (pos.row >= static_cast<int>(sheet_.size())) {
        return nullptr;
    }
    if (pos.col >= static_cast<int>(sheet_[pos.row].size())) {
        return nullptr;
    }

    return sheet_[pos.row][pos.col].get();
}

void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid()) {
        std::throw_with_nested(InvalidPositionException("Invalid position"s));
    }

    // Если ячейка существует в таблице очищаем ее и обновляем размер
    if (pos.row < static_cast<int>(sheet_.size()) && pos.col < static_cast<int>(sheet_[pos.row].size())) {
        sheet_[pos.row][pos.col].reset();

        // Обновляем размер
        int printable_col = -1;
        int printable_row = -1;
        for (int i = 0; i < static_cast<int>(sheet_.size()); i++) {
            for (int j = 0; j < printable_size_.cols; j++) {
                if (j < static_cast<int>(sheet_[i].size())) {
                    if (sheet_[i][j]) {
                        if (i > printable_row) {
                            printable_row = i;
                        }
                        if (j > printable_col) {
                            printable_col = j;
                        }
                    }
                }
            }
        }
        printable_size_.rows = printable_row + 1;
        printable_size_.cols = printable_col + 1;
    }
}

Size Sheet::GetPrintableSize() const {
    return printable_size_;
}

void Sheet::PrintValues(std::ostream& output) const {
    for (int i = 0; i < printable_size_.rows; i++) {
        for (int j = 0; j < printable_size_.cols; j++) {
            if (j != 0) {
                output << '\t';
            }
            if (j < static_cast<int>(sheet_[i].size())) {
                if (sheet_[i][j]) {
                    const auto value = sheet_[i][j]->GetValue();
                    if (std::holds_alternative<FormulaError>(value)) {
                        output << std::get<FormulaError>(value);
                    }
                    else if (std::holds_alternative<std::string>(value)) {
                        output <<  std::get<std::string>(value);
                    }
                    else {
                        output << std::get<double>(value);
                    }
                }
            }
        }
        output << '\n';
    }
}

void Sheet::PrintTexts(std::ostream& output) const {
    for (int i = 0; i < printable_size_.rows; i++) {
        for (int j = 0; j < printable_size_.cols; j++) {
            if (j != 0) {
                output << '\t';
            }
            if (j < static_cast<int>(sheet_[i].size())) {
                if (sheet_[i][j]) {
                    output << sheet_[i][j]->GetText();
                }
            }
        }
        output << '\n';
    }
}

const Cell *Sheet::GetConcreteCell(Position pos) const
{
    if (!pos.IsValid()) {
        std::throw_with_nested(InvalidPositionException("Invalid position"s));
    }

    // Если ячейки с заданным индексом не существует, возвращаем nullptr
    if (pos.row >= static_cast<int>(sheet_.size())) {
        return nullptr;
    }
    if (pos.col >= static_cast<int>(sheet_[pos.row].size())) {
        return nullptr;
    }

    return sheet_[pos.row][pos.col].get();
}

Cell *Sheet::GetConcreteCell(Position pos)
{
    if (!pos.IsValid()) {
        std::throw_with_nested(InvalidPositionException("Invalid position"s));
    }

    // Если ячейки с заданным индексом не существует, возвращаем nullptr
    if (pos.row >= static_cast<int>(sheet_.size())) {
        return nullptr;
    }
    if (pos.col >= static_cast<int>(sheet_[pos.row].size())) {
        return nullptr;
    }

    return sheet_[pos.row][pos.col].get();
}

void Sheet::CheckCircularDependencies(Position find_pos, const Cell* cell) const
{
    std::unordered_set<const Cell*> visited;
    for (const auto& referenced_pos : cell->GetReferencedCells()) {
        auto referenced_cell = GetConcreteCell(referenced_pos);
        if (referenced_cell) {
            if (visited.count(referenced_cell)) {
                continue;
            }
            visited.insert(referenced_cell);
            if (referenced_pos == find_pos) {
                std::throw_with_nested(CircularDependencyException("Circular dependency!"s));
            }
            CheckCircularDependencies(find_pos, referenced_cell, visited);
        }
    }
}

void Sheet::CheckCircularDependencies(Position find_pos, const Cell *cell, std::unordered_set<const Cell *> &visited) const
{
    for (const auto& referenced_pos : cell->GetReferencedCells()) {
        auto referenced_cell = GetConcreteCell(referenced_pos);
        if (visited.count(referenced_cell)) {
            continue;
        }
        visited.insert(referenced_cell);
        if (referenced_pos == find_pos) {
            std::throw_with_nested(CircularDependencyException("Circular dependency!"s));
        }
        CheckCircularDependencies(find_pos, referenced_cell, visited);
    }
}

void Sheet::AddDependentsCells(Position pos, const std::vector<Position>& referenced_cells)
{
    for (const auto& referenced_pos : referenced_cells) {
        if (referenced_pos == pos) {
            std::throw_with_nested(CircularDependencyException("Circular dependency!"s));
        }
        if (!GetCell(referenced_pos)) {
            SetEmptyCell(referenced_pos);
        }
        GetConcreteCell(referenced_pos)->AddDependentCell(pos);
    }
}

void Sheet::CopyDependentsCells(Cell *cell, const std::unordered_set<Position, Position::PositionHasher> &dependent_cells)
{
    for (const auto& dependent_pos : dependent_cells) {
        cell->AddDependentCell(dependent_pos);
    }
}

void Sheet::RemoveDependentsCells(Position pos, const std::vector<Position> &referenced_cells)
{
    for (const auto& referenced_pos : referenced_cells) {
        GetConcreteCell(referenced_pos)->RemoveDependentCell(pos);
    }
}

void Sheet::SetEmptyCell(Position pos) {
    if (!pos.IsValid()) {
        std::throw_with_nested(InvalidPositionException("Invalid position"s));
    }

    if (pos.row >= static_cast<int>(sheet_.size())) {
        sheet_.resize(pos.row + 1);
    }
    if (pos.col >= static_cast<int>(sheet_[pos.row].size())) {
        sheet_[pos.row].resize(pos.col + 1);
    }

    sheet_[pos.row][pos.col] = std::make_unique<Cell>(*this);
}

void Sheet::InvalidateCache(Position pos)
{
    std::unordered_set<Position, Position::PositionHasher> visited;
    InvalidateCache(pos, visited);
}

void Sheet::InvalidateCache(Position pos, std::unordered_set<Position, Position::PositionHasher>& visited)
{
    for (auto& dependent_pos : GetConcreteCell(pos)->GetDependentCells()) {
        if (visited.count(dependent_pos)) {
            continue;
        }
        visited.insert(dependent_pos);
        auto dependent_cell = GetConcreteCell(dependent_pos);
        dependent_cell->ClearCache();
        InvalidateCache(dependent_pos, visited);
    }
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}
