import importlib.util,tempfile,unittest
from pathlib import Path
spec=importlib.util.spec_from_file_location('prepare_main',Path(__file__).resolve().parents[1]/'scripts/prepare_main.py')
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)

class PromotionTest(unittest.TestCase):
    def test_blocks_removed_and_production_preserved(self):
        self.assertEqual(module.cleaned('sensor:\n# DEV_ONLY_BEGIN\n  debug: true\n# DEV_ONLY_END\n  normal: true\n'),'sensor:\n  normal: true\n')
    def test_marked_file_removed(self):
        self.assertIsNone(module.cleaned('# DEV_ONLY_FILE\nanything\n'))
    def test_cpp_blocks(self):
        self.assertEqual(module.cleaned('a\n// DEV_ONLY_BEGIN\nx\n// DEV_ONLY_END\nb\n'),'a\nb\n')
    def test_invalid_markers_fail(self):
        for text in ['# DEV_ONLY_BEGIN\nx\n','# DEV_ONLY_END\n','# DEV_ONLY_BEGIN\n# DEV_ONLY_BEGIN\n']:
            with self.subTest(text=text),self.assertRaises(ValueError):module.cleaned(text)
    def test_idempotence_and_no_changes_outside_scope(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory)
            (root/'packages').mkdir()
            (root/'packages/dev.yaml').write_text('# DEV_ONLY_FILE\nx\n')
            (root/'README.md').write_text('# DEV_ONLY_FILE\npreserved\n')
            self.assertEqual(module.promote(root,True),['packages/dev.yaml'])
            self.assertEqual(module.promote(root,True),[])
            self.assertTrue((root/'README.md').exists())
    def test_malformed_file_does_not_partially_strip(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);(root/'packages').mkdir()
            a=root/'packages/a.yaml';a.write_text('# DEV_ONLY_FILE\nx\n')
            (root/'packages/b.yaml').write_text('# DEV_ONLY_BEGIN\nx\n')
            with self.assertRaises(ValueError):module.promote(root,True)
            self.assertTrue(a.exists())

if __name__=='__main__':unittest.main()
