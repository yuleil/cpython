import unittest

from .utils import CdsTestMixin, assert_archive_created


# todo: verify verbose messages.

class CdsImportTest(CdsTestMixin, unittest.TestCase):
    @assert_archive_created
    def test_dump_import(self):
        self.assert_python_ok(
            '-c', f'import json',
            **self.get_cds_env(1, self.TEST_ARCHIVE, 2))
        out = self.assert_python_ok(
            '-c', f'import sys; print(sys.shm_getobj().keys())',
            **self.get_cds_env(10, self.TEST_ARCHIVE, 2))
        self.assertIn("'json'", out.out.decode())

    @assert_archive_created
    def test_import(self):
        self.assert_python_ok(
            '-c', f'import json',
            **self.get_cds_env(1, self.TEST_ARCHIVE, 2))
        self.assert_python_ok(
            '-c', f'import json',
            **self.get_cds_env(2, self.TEST_ARCHIVE, 2))
