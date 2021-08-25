import unittest

from .utils import CdsTestMixin, assert_archive_created


class CdsArchiveTest(CdsTestMixin, unittest.TestCase):
    TEST_ARCHIVE: str

    @assert_archive_created
    def test_create_archive(self):
        self.assert_python_ok(
            '-c', 'import logging,json,decimal',
            **self.get_cds_env(1, self.TEST_ARCHIVE, 0))
