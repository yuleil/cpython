import unittest

from .utils import CdsTestMixin, assert_archive_created


class CdsSerializeCodeTest(CdsTestMixin, unittest.TestCase):
    TEST_ARCHIVE: str

    @assert_archive_created
    def test_archive_lambda(self):
        def test_archive_code(o):
            r = repr(o)

            self.assert_python_ok(
                '-c', f'import sys; sys.shm_move_in((lambda: print(repr({r}))).__code__)',
                **self.get_cds_env(9, self.TEST_ARCHIVE, 0))

            out = self.assert_python_ok(
                '-c', 'import sys; exec(sys.shm_getobj())',
                **self.get_cds_env(10, self.TEST_ARCHIVE, 0))
            self.assertEqual(r, out.out.decode().strip())

        test_archive_code("a" * 100)
        test_archive_code(0)
        test_archive_code(b'')
