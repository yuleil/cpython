import unittest

from . import CdsTestMixin


class CdsArchiveTest(CdsTestMixin, unittest.TestCase):
    TEST_ARCHIVE: str

    def test_create_archive(self):
        self.assertNotExists(self.TEST_ARCHIVE)

        self.assert_python_ok(
            '-c', 'import logging,json,decimal',
            **self.get_cds_env(1, self.TEST_ARCHIVE, 0))

        self.assertExists(self.TEST_ARCHIVE)
