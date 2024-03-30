#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>
#include <vector>

using namespace std::literals;


Sheet::~Sheet() {}

void Sheet::SetCell(Position pos, std::string text) {

    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid table position");
    }

    SetTableSize(pos);
  
    if (table_[pos.row][pos.col] == nullptr){
        table_[pos.row][pos.col] = ( std::make_unique<Cell>(*this));
    }

    table_[pos.row][pos.col]->Set(text);
}

const CellInterface* Sheet::GetCell(Position pos) const {
    return GetConcreteCell(pos);
}

CellInterface* Sheet::GetCell(Position pos) {
    return GetConcreteCell(pos);
}

const Cell* Sheet::GetConcreteCell(Position pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid table position");
    }

    if (pos.row + 1 > static_cast<int>(table_.size()) || pos.col + 1 > static_cast<int>(table_[pos.row].size())) {
        return nullptr;
    }
   const Cell* cell = table_[pos.row][pos.col].get();
    return cell;

}

Cell* Sheet::GetConcreteCell(Position pos) {
    return const_cast<Cell*>(
         static_cast<const Sheet&>(*this).GetConcreteCell(pos));

}


void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid table position");
    }

   if (pos.row + 1 > static_cast<int>(table_.size()) || pos.col + 1 > static_cast<int>(table_[pos.row].size())){
        return;
    }

    if (table_[pos.row][pos.col] != nullptr) {
        table_[pos.row][pos.col] = nullptr;
    }
}

Size Sheet::GetPrintableSize() const {
    Size result{ 0, 0 };

    if (table_.size() == 0) {
        return result;
    }

    for (int i = table_.size()-1; i >= 0 ; --i) {
        int cols = table_[i].size();
        for (int j = table_[i].size() -1 ; j >= 0; --j) {
          if (table_[i][j] != nullptr) {
              if (cols != 0) {
                  result.rows = std::max( i+1, result.rows);
              }

              if (result.cols < cols) {
                  result.cols = cols;
              }
              break;
          } else {
              --cols;
          }
        }
    }
    return {result};
}

void Sheet::PrintValues(std::ostream& output) const {
    Size size = GetPrintableSize();
    for (int i = 0; i < size.rows; ++i) {
        bool first = true;
        for (int j = 0; j < size.cols; ++j) {
            if (!first) {
                output << '\t';
            }
            first = false;
            if (table_[i][j] != nullptr) {
                auto value = table_[i][j]->GetValue();
                std::visit([&output](auto&& arg) {output << arg; }, value);
            }
        }
        output << '\n';
    }
}

void Sheet::PrintTexts(std::ostream& output) const {
    Size size = GetPrintableSize();
    for (int i = 0; i < size.rows; ++i) {
        bool first = true;
        for (int j = 0; j < size.cols; ++j) {
            if (!first) {
                output << "\t";
            };
            first = false;
            if (table_[i][j] != nullptr) {
                auto text = table_[i][j]->GetText();
                output << text;
            }
        }
        output  << "\n";
    }
}

void Sheet::SetTableSize(Position pos) {

    if (static_cast<int>(table_.size()) < pos.row + 1) {
        for (int i = static_cast<int>(table_.size()); i < pos.row + 1; ++i) {
            std::vector<std::unique_ptr<Cell>> row(pos.col + 1);
            table_.push_back(std::move(row));
        }
    }

    for (int i = 0; i < static_cast<int>(table_.size()); ++i) {
        while (static_cast<int>(table_[i].size()) < std::max(static_cast<int>(table_[0].size()), pos.col + 1)) {
            table_[i].push_back(nullptr);
        }
    }
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}