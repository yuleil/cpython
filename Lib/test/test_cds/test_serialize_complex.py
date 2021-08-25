import unittest

from .utils import assert_archive_created
from .test_serialize_basic import SerializeTestMixin


class CdsSerializeComplexTest(SerializeTestMixin, unittest.TestCase):
    @assert_archive_created
    def test_empty_dict(self):
        self.run_serialize_test({})

    @assert_archive_created
    def test_dict_int(self):
        self.run_serialize_test({1: 1})

    @assert_archive_created
    def test_dict_str(self):
        self.run_serialize_test({'1': b'1'})

    @assert_archive_created
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
