import functools
import os
import random
import unittest

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


def assert_archive_created(f: t.Callable):
    @functools.wraps(f)
    def inner(self: unittest.TestCase):
        self.assertIsInstance(self, CdsTestMixin)
        self.assertNotExists(self.TEST_ARCHIVE)

        f(self)

        self.assertExists(self.TEST_ARCHIVE)

    return inner


def random_branch():
    return random.choice([True, False])


def random_float():
    f = random.random()
    if f == 0.0:
        return f
    if random_branch():
        f = 1 / f
    return f
