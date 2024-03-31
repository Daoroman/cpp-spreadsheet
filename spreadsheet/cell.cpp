#include "cell.h"
#include "sheet.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>


class Cell::Impl {
public:
	virtual ~Impl() = default;
	virtual Value GetValue() const = 0;
	virtual std::string GetText() const = 0;
	virtual std::vector<Position> GetReferencedCells() const { return {}; }
	virtual bool IsCacheValid() const { return true; }
	virtual void InvalidateCache() {}
};

class Cell::EmptyImpl : public Impl {
public:
	Value GetValue() const override { return ""; }
	std::string GetText() const override { return ""; }
};

class Cell::TextImpl : public Impl {
public:
	TextImpl(std::string text)
		: text_(std::move(text)) {
		if (text_.empty()) { throw std::logic_error(""); }
	}

	Value GetValue() const override {
		if (text_[0] == ESCAPE_SIGN) return text_.substr(1);
		return text_;
	}

	std::string GetText() const override {
		return text_;
	}

private:
	std::string text_;
};

class Cell::FormulaImpl : public Impl {
public:
	explicit FormulaImpl(std::string expression, const SheetInterface& sheet)
		: sheet_(sheet) {
		if (expression.empty() || expression[0] != FORMULA_SIGN) throw std::logic_error("");

		formula_ptr_ = ParseFormula(expression.substr(1));
	}

	Value GetValue() const override {
		if (!cache_) cache_ = formula_ptr_->Evaluate(sheet_);

		auto value = formula_ptr_->Evaluate(sheet_);
		if (std::holds_alternative<double>(value)) return std::get<double>(value);

		return std::get<FormulaError>(value);
	}

	std::string GetText() const override {
		return FORMULA_SIGN + formula_ptr_->GetExpression();
	}

	bool IsCacheValid() const override {
		return cache_.has_value();
	}

	void InvalidateCache() override {
		cache_.reset();
	}

	std::vector<Position> GetReferencedCells() const {
		return formula_ptr_->GetReferencedCells();
	}

private:
	std::unique_ptr<FormulaInterface> formula_ptr_;
	const SheetInterface& sheet_;
	mutable std::optional<FormulaInterface::Value> cache_;
};

Cell::Cell(Sheet& sheet)
	: impl_(std::make_unique<EmptyImpl>())
	, sheet_(sheet)
{
}

Cell::~Cell() {}

void Cell::CashInvalidation(bool recursive){
	if (impl_->IsCacheValid() || recursive) {
		impl_->InvalidateCache();
		for (Cell* cell : left_cells_) {
			cell->CashInvalidation();
		}
	}
}

void Cell::CircularDependency(const Impl& impl) {
	if (impl.GetReferencedCells().empty()) {
		return;
	}

	std::unordered_set<const Cell*> referenced;
	for (const auto& pos : impl.GetReferencedCells()) {
		auto cell = sheet_.GetConcreteCell(pos);
		referenced.insert(cell);
	}
	std::unordered_set<const Cell*> visited;
	std::stack<const Cell*> to_visit;
	to_visit.push(this);
	while (!to_visit.empty()) {
		const Cell* current = to_visit.top();
		to_visit.pop();
		visited.insert(current);

		if (referenced.find(current) != referenced.end()) {
			throw CircularDependencyException("");
		}

		for (const Cell* incoming : current->left_cells_) {
			if (visited.find(incoming) == visited.end()) to_visit.push(incoming);
		}
	}
}

void Cell::Set(std::string text) {
	std::unique_ptr<Impl> impl;

	if (text.size() == 0) {
		impl = std::make_unique<EmptyImpl>();
	}
	else if (text[0] == FORMULA_SIGN && text.size() > 1) {
		impl = std::make_unique<FormulaImpl>(std::move(text), sheet_);
	}
	else
			(impl = std::make_unique<TextImpl>(std::move(text)));

	CircularDependency(*impl);

	impl_ = std::move(impl);

	for (Cell* cell : right_cells_) {
		cell->left_cells_.erase(this);
	}

	right_cells_.clear();

	for (const auto& pos : impl_->GetReferencedCells()) {
		Cell* cell = sheet_.GetConcreteCell(pos);
		if (!cell) {
			sheet_.SetCell(pos, "");
			cell = sheet_.GetConcreteCell(pos);
		}
		right_cells_.insert(cell);
		cell->left_cells_.insert(this);
	}
	CashInvalidation(true);
}

void Cell::Clear() {
	Set("");
}

Cell::Value Cell::GetValue() const {
	return impl_->GetValue();
}
std::string Cell::GetText() const {
	return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const {
	return impl_->GetReferencedCells();
}

bool Cell::IsReferenced() const {
	return !left_cells_.empty();
}