from init_lib.utils import tupleassert, rtest


class TestSeperated:
    def cmm1(self):
        tupleassert(
            rtest("./build/src/parser ./Original-Lab/Test/test1.cmm"), ("", ""))
