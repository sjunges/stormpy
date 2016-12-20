#include "common.h"

#include "core/core.h"
#include "core/modelchecking.h"
#include "core/bisimulation.h"
#include "core/input.h"

PYBIND11_PLUGIN(core) {
    py::module m("core");
    
#ifdef STORMPY_DISABLE_SIGNATURE_DOC
    py::options options;
    options.disable_function_signatures();
#endif
    
    
    define_core(m);
    define_parse(m);
    define_build(m);
    define_modelchecking(m);
    define_bisimulation(m);
    define_input(m);
    return m.ptr();
}