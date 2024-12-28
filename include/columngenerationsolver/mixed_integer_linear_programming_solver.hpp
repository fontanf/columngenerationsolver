#pragma once

#include <istream>

#if CBC_FOUND
#include <coin/CbcModel.hpp>
#include <coin/CbcOrClpParam.hpp>
#include <coin/OsiCbcSolverInterface.hpp>
#endif

namespace columngenerationsolver
{

enum class MilpSolverName { CBC, Highs };

inline std::istream& operator>>(
        std::istream& in,
        MilpSolverName& solver_name)
{
    std::string token;
    in >> token;
    if (token == "cbc" || token == "CBC") {
        solver_name = MilpSolverName::CBC;
    } else if (token == "highs" || token == "Highs" || token == "HIGHS") {
        solver_name = MilpSolverName::Highs;
    } else  {
        in.setstate(std::ios_base::failbit);
    }
    return in;
}

inline std::ostream& operator<<(
        std::ostream& os,
        MilpSolverName solver_name)
{
    switch (solver_name) {
    case MilpSolverName::CBC: {
        os << "CBC";
        break;
    } case MilpSolverName::Highs: {
        os << "Highs";
        break;
    }
    }
    return os;
}

inline MilpSolverName s2milps(const std::string& s)
{
    if (s == "cbc" || s == "CBC") {
        return MilpSolverName::CBC;
    } else if (s == "hights" || s == "Highs" || s == "HIGHS") {
        return MilpSolverName::Highs;
    } else {
        return MilpSolverName::CBC;
    }
}

}
