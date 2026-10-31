# Diagnostic des coupures et des redémarrages

`packages/dev-diagnostics.yaml` est réservé au développement. Les six anciennes
sondes d'humidité temporaires sont également balisées. La régulation, le polling
iBUS, l'économie Wi-Fi et les délais de redémarrage Wi-Fi/API sont conservés pour
observer le défaut sans en changer les conditions.

## Mesures et interprétation

- Démarrages persistants : compteur NVS dédié, synchronisé immédiatement une fois
  par boot. Il commence avec le premier firmware instrumenté, inclut les mises à
  jour et ne peut compter un reset antérieur à son initialisation.
- Durée depuis démarrage, motif du dernier reset, signal Wi-Fi, mémoire libre et
  minimale, durée maximale de boucle : composants natifs uptime/wifi_signal/debug.
- Reconnexions Wi-Fi : première connexion exclue. Déconnexions Wi-Fi : pertes de
  connexion. Ces compteurs repartent de zéro après un redémarrage.
- Connexions API tous clients : inclut les lecteurs de logs. Les compteurs de
  reconnexions/déconnexions Home Assistant filtrent le préfixe `Home Assistant`
  dans client_info ; les lecteurs de logs ne sont donc pas comptés comme HA.
  La première connexion HA du boot n'est pas une reconnexion. Plusieurs instances
  HA seraient comptées ensemble.

Les compteurs réseau restent en RAM pour éviter les écritures flash à chaque
coupure. Ils sont publiés toutes les 10 s, les autres mesures toutes les 30 s.

Uptime continu + Wi-Fi qui monte : perte Wi-Fi sans reboot. Uptime continu +
Wi-Fi stable + reconnexions HA : liaison API. Boots qui monte + uptime remis à
zéro : redémarrage ; le motif peut orienter vers watchdog, logiciel, alimentation.

Pour les événements de logs, régler le logger **du YAML appareil** sur INFO en
conservant baud_rate: 0, car UART0 sert au bus. Son niveau ERROR est prioritaire
sur un package. Les diagnostics fonctionnent même si le logger reste sur ERROR.

## Retrait automatique dans main

`python scripts/prepare_main.py --strip` retire les blocs DEV_ONLY_BEGIN/END
(commentaires # ou //), les fichiers commençant par DEV_ONLY_FILE et replace
la référence du composant externe sur main. Il ne touche que packages/, components/
et examples/. Des marqueurs invalides font échouer avant toute écriture.
`--check` vérifie sans modifier. Le retrait est idempotent.

Le workflow prepare-main.yml, **une fois fusionné dans main**, effectue ce retrait
après chaque push/merge et enregistre automatiquement le résultat. Main peut donc
brièvement contenir les diagnostics pendant le job. Pour éviter cette fenêtre,
exécuter le script sur la branche de promotion avant fusion. Si les protections de
branche interdisent les pushes du bot, le job échoue explicitement : autoriser le
bot ou préparer la branche de promotion. Les branches dev ne sont pas réécrites.

Le CI valide et compile les profils development et production. Les profils de
`.ci-build/` utilisent des secrets factices et ne doivent **jamais être flashés**.
L'installation réelle doit utiliser le YAML et les secrets actuels de l'appareil,
avec sa référence de package sur la branche instrumentée.

```sh
python -m unittest discover -s tests -v
python scripts/prepare_ci.py
esphome config .ci-build/development/device.yaml
python scripts/prepare_ci.py --production
esphome config .ci-build/production/device.yaml
```

Références : https://esphome.io/components/wifi/,
https://esphome.io/components/api/, https://esphome.io/components/debug/.

## Versions des branches

La substitution `firmware_version` dans `packages/easyhome.yaml` alimente à la
fois le projet ESPHome et le capteur « Version firmware Aldes ». Les branches
hors `main` utilisent le suffixe `-dev` (par exemple `1.2.0-dev`).

Le workflow `prepare-dev-version.yml` est déclenché à la création d'une branche,
à chaque push hors main, et manuellement. Il applique le suffixe une seule fois
et vérifie les versions avant de committer uniquement les deux fichiers concernés.
Les tags et main sont exclus. Le workflow doit exister dans la branche créée ;
pour les branches futures issues de main, il devra donc d'abord y être intégré.
Un push produit avec GITHUB_TOKEN ne redéclenche pas les workflows : la validation
complète du commit automatique pourra être lancée manuellement si nécessaire.

`python scripts/prepare_dev_version.py --branch NOM` prépare une branche locale ;
`--check` vérifie la cohérence sans écrire. Le nettoyage de production retire
également le suffixe des déclarations de version. Le profil CI production utilise
donc une version stable, tandis que le profil développement conserve `-dev`.
La vérification d'une release exige une version stable correspondant au tag.

Un appareil déjà flashé continue d'annoncer son ancienne version : une modification
GitHub ne modifie pas le firmware en service. Il faudra préparer puis autoriser
un prochain flashage pour voir le suffixe dans Home Assistant.
