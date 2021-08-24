import unittest

from . import CdsTestMixin


class SerializeTestMixin(CdsTestMixin):
    def run_serialize_test(self, value):
        self.assertNotExists(self.TEST_ARCHIVE)

        s = repr(value)

        self.assert_python_ok(
            '-c', f'import sys; sys.shm_move_in({s})',
            **self.get_cds_env(9, self.TEST_ARCHIVE, 0))

        out = self.assert_python_ok(
            '-c', 'import sys; print(repr(sys.shm_getobj()))',
            **self.get_cds_env(10, self.TEST_ARCHIVE, 0))
        self.assertEqual(s, out.out.decode().strip())

        self.assertExists(self.TEST_ARCHIVE)


class CdsSerializeBasicTest(SerializeTestMixin, unittest.TestCase):
    TEST_ARCHIVE: str

    def test_none(self):
        self.run_serialize_test(None)

    def test_long(self):
        self.run_serialize_test(2 ** 100)

    def test_bool_true(self):
        self.run_serialize_test(True)

    def test_bool_false(self):
        self.run_serialize_test(False)

    def test_empty_string(self):
        self.run_serialize_test("")

    def test_string_1(self):
        self.run_serialize_test("a" * 10000)

    def test_float(self):
        self.run_serialize_test(1.3)

    def test_complex(self):
        self.run_serialize_test(3 + 4j)

    def test_bytes(self):
        self.run_serialize_test(b'a' * 10000)
