import os

from test.support import load_package_tests
from test.support.script_helper import assert_python_ok

import typing as t


class UtilMixin:
    exists = staticmethod(os.path.exists)
    assert_python_ok = staticmethod(assert_python_ok)

    assertExists = lambda self, file: self.assertTrue(self.exists(file))
    assertNotExists = lambda self, file: self.assertFalse(self.exists(file))


class CdsTestMixin(UtilMixin):
    TEST_ARCHIVE = 'test.img'

    def _del_archive(self):
        if os.path.exists(self.TEST_ARCHIVE):
            os.remove(self.TEST_ARCHIVE)

    setUp = _del_archive
    tearDown = _del_archive

    @staticmethod
    def get_cds_env(mode: t.Union[str, int], archive: str, verbose: t.Union[str, int],
                    random_hash_seed: t.Union[bool, int, str] = False):
        env = os.environ.copy()
        env['__cleanenv'] = True  # signal to assert_python not to do a copy
        # of os.environ on its own

        env['PYTHONHASHSEED'] = 'random' if random_hash_seed in (True, 'random') else \
            str(random_hash_seed) if random_hash_seed else '0'

        env['PYCDSMODE'] = str(mode)
        env['PYCDSARCHIVE'] = archive
        env['PYCDSVERBOSE'] = str(verbose)

        return env


def load_tests(*args):
    return load_package_tests(os.path.dirname(__file__), *args)
