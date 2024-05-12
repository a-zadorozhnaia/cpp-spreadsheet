#pragma once

#include "cell.h"
#include "common.h"

#include <functional>

class Sheet : public SheetInterface {
public:
    ~Sheet();

    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

private:
    const Cell* GetConcreteCell(Position pos) const;
    Cell* GetConcreteCell(Position pos);

    void CheckCircularDependencies(Position find_pos, const Cell *cell) const;
    void CheckCircularDependencies(Position find_pos, const Cell *cell, std::unordered_set<const Cell*>& visited) const;

    void AddDependentsCells(Position pos, const std::vector<Position>& referenced_cells);
    void CopyDependentsCells(Cell * cell, const std::unordered_set<Position, Position::PositionHasher> & dependent_cells);
    void RemoveDependentsCells(Position pos, const std::vector<Position>& referenced_cells);

    void SetEmptyCell(Position pos);

    void InvalidateCache(Position pos);
    void InvalidateCache(Position pos, std::unordered_set<Position, Position::PositionHasher> &visited);

    Size printable_size_{0, 0};
    std::vector<std::vector<std::unique_ptr<Cell>>> sheet_;
};
