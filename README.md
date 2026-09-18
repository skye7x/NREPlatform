# NREPlatform
## Budowanie

```bash
make
```

### Usługi systemd
```bash
sudo systemctl start nre        # uruchom daemon
sudo systemctl enable nre       # autostart po starcie systemu
sudo systemctl start nre-api    # uruchom REST API
```

## Użycie

### Eksperymenty
```bash
nre start --iface eth0 --target-ip 192.168.1.50 --latency 100 --jitter 30 --loss 3 --download 10000 --upload 0 --duration 120
nre dns-failure --target-ip 192.168.1.50 --duration 30
nre disconnect --target-ip 192.168.1.50 --duration 10
```

### Profile
```bash
nre profile save --profile-name "Poor Mobile" --latency 100 --jitter 30 --loss 2 --download 10000 --duration 120
nre profile start --profile-name "Poor Mobile" --iface eth0
nre profile list
```

### Polityki
```bash
nre policy save --policy-name "high_latency" --logic AND --action trigger_alert --cond1-metric latency --cond1-op ">" --cond1-value 100
nre policy test --policy-name "high_latency" --latency 150
```

### Chain
```bash
nre chain save --chain-name "gradual" --profile1 "step1" --profile2 "step2" --profile3 "step3"
nre chain start --chain-name "gradual" --iface eth0
```

### Presety
```bash
nre preset list
nre preset info --preset-name weak_wifi
nre preset export --preset-name weak_wifi --output weak_wifi.json
nre preset export-all --output all_presets.json
nre preset import --input my_custom_preset.json
```

### Harmonogram
```bash
nre schedule add --profile-name "Poor Mobile" --iface eth0 --time 18:00 --repeat
nre schedule list
nre schedule remove --id 1
```

### Daemon
```bash
nre daemon start --target 192.168.1.50 --iface eth0 --interval 60
nre daemon stop
nre daemon status
```

### API
```bash
nre api start --port 8080
```

### Użytkownicy
```bash
nre user add --username admin --password secret --level admin
nre user list
nre user remove --username test
```

### Historia
```bash
nre history list --limit 10
nre history export --format json --output results.json
nre audit list
nre audit export --output audit.json
```

### Status
```bash
nre status
```

### Test Automation
```bash
nre test run --profile-name "Poor Mobile" --iface eth0 --duration 120
nre test list
```

## Licencja

GNU Affero General Public License v3 (AGPL-3.0)
