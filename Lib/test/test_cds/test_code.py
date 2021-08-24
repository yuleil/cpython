import unittest

from . import CdsTestMixin


class CdsSerializeBasicTest(CdsTestMixin, unittest.TestCase):
    TEST_ARCHIVE: str

    def test_import(self):
        self.assertNotExists(self.TEST_ARCHIVE)

        self.assert_python_ok(
            '-c', f'import json',
            **self.get_cds_env(1, self.TEST_ARCHIVE, 0))

        out = self.assert_python_ok(
            '-c', 'import sys; print(repr(list(sys.shm_getobj().keys())))',
            **self.get_cds_env(2, self.TEST_ARCHIVE, 0))
        self.assertIn('json', eval(out.out.decode().strip()))

        self.assertExists(self.TEST_ARCHIVE)
