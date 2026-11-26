#ifndef PLOT_H
#define PLOT_H

#include <stdexcept>
#include <string>

#include "ast.h"
#include "eval.h"

// Exceção do Plot
class PlotError : public std::runtime_error {
public:
	explicit PlotError(const std::string &msg) : std::runtime_error(msg) {}
};

void plot_async(std::string &input);
void plot_wait();

#endif