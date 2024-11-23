#pragma once

#include "columngenerationsolver/commons.hpp"

namespace columngenerationsolver
{

class AlgorithmFormatter
{

public:

    /** Constructor. */
    AlgorithmFormatter(
            const Model& model,
            const Parameters& parameters,
            Output& output):
        model_(model),
        parameters_(parameters),
        output_(output),
        os_(parameters.create_os()) { }

    /** Print the header. */
    void start(
            const std::string& algorithm_name);

    /** Print the header of the column generation algorithm. */
    void print_column_generation_header();

    /** Print current iteration of the column generation algorithm. */
    void print_column_generation_iteration(
            Counter iteration,
            ColIdx number_of_columns,
            double value,
            double bound);

    /** Print the header. */
    void print_header();

    /** Print current state. */
    void print(
            const std::string& s);

    /** Update the solution. */
    void update_solution(
            const Solution& solution);

    /** Update the bound. */
    void update_bound(
            double bound);

    /** Method to call at the end of the algorithm. */
    void end();

private:

    /*
     * Private methods
     */

    /*
     * Private attributes
     */

    const Model& model_;

    /** Parameters. */
    const Parameters& parameters_;

    /** Output. */
    Output& output_;

    /** Output stream. */
    std::unique_ptr<optimizationtools::ComposeStream> os_;

};

}
