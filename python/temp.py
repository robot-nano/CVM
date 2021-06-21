import cvm._ffi
from cvm.runtime import Object


@cvm._ffi.register_object("test.temp")
class ModularSet(Object):
    """Represent range of (coeff * x + base) for x in Z"""

    def __init__(self, coeff, base):
        pass
