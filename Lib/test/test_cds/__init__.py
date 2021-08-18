import os
import unittest

from test.support.script_helper import assert_python_ok

import typing as t


class CdsTestMixin:
    TEST_ARCHIVE = 'test.img'

    def _del_archive(self):
        if os.path.exists(self.TEST_ARCHIVE):
            os.remove(self.TEST_ARCHIVE)

    setUp = _del_archive
    tearDown = _del_archive


class CdsTest(CdsTestMixin, unittest.TestCase):
    TEST_ARCHIVE: str

    @staticmethod
    def get_cds_env(mode: t.Union[str, int], archive: str, verbose: t.Union[str, int]):
        env = os.environ.copy()
        env['__cleanenv'] = True  # signal to assert_python not to do a copy
        # of os.environ on its own
        env['PYTHONHASHSEED'] = '0'

        env['PYCDSMODE'] = str(mode)
        env['PYCDSARCHIVE'] = archive
        env['PYCDSVERBOSE'] = str(verbose)

        return env

    def test_create_archive(self):
        self.assertFalse(os.path.exists(self.TEST_ARCHIVE))

        assert_python_ok(
            '-c', 'import logging,json,decimal',
            **self.get_cds_env(1, self.TEST_ARCHIVE, 0))

        self.assertTrue(os.path.exists(self.TEST_ARCHIVE))
