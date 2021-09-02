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

    @assert_archive_created
    def test_import_subpackage_not_in_archive(self):
        self.assert_python_ok(
            '-c', f'import xml',
            **self.get_cds_env(1, self.TEST_ARCHIVE, 2))
        self.assert_python_ok(
            '-c', f'import xml.dom',
            **self.get_cds_env(2, self.TEST_ARCHIVE, 2))

    def test_cached_import_result(self):
        printer = "import xml\nfor i in ('__package__', '__file__', '__path__'):\n    print(xml.__dict__.get(i, None))"
        trace_out = self.assert_python_ok(
            '-c', printer,
            **self.get_cds_env(1, self.TEST_ARCHIVE, 2))
        replay_out = self.assert_python_ok(
            '-c', printer,
            **self.get_cds_env(2, self.TEST_ARCHIVE, 2))
        self.assertEqual(trace_out.out.decode(), replay_out.out.decode())

    def test_archive_not_exist_warning(self):
        out = self.assert_python_ok(
            '-c', '',
            **self.get_cds_env(2, self.TEST_ARCHIVE, 2))
        self.assertIn('[sharedheap] open mmap file failed.', out.err.decode())
