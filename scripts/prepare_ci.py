"""Prepare non-secret offline dev/production profiles for ESPHome validation."""
from pathlib import Path
import argparse,re,shutil
from prepare_main import promote

root=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser();parser.add_argument('--production',action='store_true');args=parser.parse_args()
target=root/'.ci-build'/('production' if args.production else 'development')
(target/'packages').mkdir(parents=True,exist_ok=True)
for path in (root/'packages').glob('*.yaml'):shutil.copy2(path,target/'packages'/path.name)
if args.production:promote(target,strip=True)
package=target/'packages/easyhome.yaml'
text=package.read_text(encoding='utf-8')
text=re.sub(r'type: git\s+url: https://github.com/Fredzxda/esphome-aldes-easyhome\s+ref: [^\n]+',
            'type: local\n      path: '+(root/'components').as_posix(),text)
package.write_bytes(text.encode())
example=(root/'examples/aldes-easyhome.yaml').read_text(encoding='utf-8')
example=example.replace('easyhome: github://Fredzxda/esphome-aldes-easyhome/packages/easyhome.yaml@main','easyhome: !include packages/easyhome.yaml')
if not args.production:example=example.replace('level: ERROR','level: INFO')
(target/'device.yaml').write_bytes(example.encode())
(target/'secrets.yaml').write_text('aldes_easyhome_api_key: "AQIDBAUGBwgJCgsMDQ4PEBESExQVFhcYGRobHB0eHyA="\nwifi_ssid: "CI-test"\nwifi_password: "ci-placeholder-password"\n')
print((target/'device.yaml').relative_to(root).as_posix())
