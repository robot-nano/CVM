import cvm


def test_get_global():
    targs = (10, 10.0, "hello")

    @cvm.register_func
    def my_packed_func(*args):
        assert tuple(args) == targs
        return 10

    f = cvm.get_global_func("my_packed_func")
    assert isinstance(f, cvm.runtime.PackedFunc)
    y = f(*targs)
    assert y == 10


test_get_global()
