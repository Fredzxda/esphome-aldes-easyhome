import importlib.util
from pathlib import Path
import shutil
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
def load(name):
    spec = importlib.util.spec_from_file_location(name, ROOT / 'scripts' / (name + '.py'))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module

dev = load('prepare_dev_version')
production = load('prepare_main')

class DevelopmentVersionTests(unittest.TestCase):
    def fixture(self, root):
        for folder in ('packages', 'examples'):
            shutil.copytree(ROOT / folder, root / folder)

    def test_development_production_and_round_trip(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            self.fixture(root)
            dev.prepare(root)
            version = dev.versions(root)
            self.assertTrue(version.endswith('-dev'))
            self.assertEqual(dev.prepare(root), [])
            production.promote(root, True)
            self.assertEqual(dev.versions(root), version.removesuffix('-dev'))
            self.assertEqual(production.promote(root, True), [])
            dev.prepare(root)
            self.assertEqual(dev.versions(root), version)

    def test_mismatch_is_rejected_without_partial_writes(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            self.fixture(root)
            example = root / 'examples/aldes-easyhome.yaml'
            example.write_text(example.read_text(encoding='utf-8').replace('v1.2.0', 'v9.9.9'), encoding='utf-8')
            package = root / 'packages/easyhome.yaml'
            original = package.read_bytes()
            with self.assertRaises(ValueError):
                dev.prepare(root)
            self.assertEqual(package.read_bytes(), original)

    def test_no_longer_hardcoded_sensor(self):
        package = (ROOT / 'packages/easyhome.yaml').read_text(encoding='utf-8')
        self.assertIn('return {"${firmware_version}"};', package)
        self.assertIn('version: ${firmware_version}', package)

if __name__ == '__main__':
    unittest.main()
