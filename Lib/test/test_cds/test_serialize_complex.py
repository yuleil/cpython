import unittest
from .test_serialize_basic import SerializeTestMixin


class CdsSerializeComplexTest(SerializeTestMixin, unittest.TestCase):
    def test_empty_dict(self):
        self.run_serialize_test({})

    def test_dict_int(self):
        self.run_serialize_test({1: 1})

    def test_dict_str(self):
        self.run_serialize_test({'1': b'1'})

    def test_dict_big(self):
        d = {}

        for i in range(1000):
            if i % 2:
                d[i] = str(i)
                d[str(i)] = i
            else:
                d[i] = str(i).encode()
                d[str(i).encode()] = i

        self.run_serialize_test(d)
