NREPlatform - Network Resilience Experimentation Platform

1. Cel projektu
NREPlatform to platforma działająca na routerze z OpenWrt, przeznaczona do kontrolowanego testowania odporności sieci oraz urządzeń na różnego rodzaju problemy z połączeniem.
Platforma pozwala administratorowi stworzyć określone warunki sieciowe, obserwować ich wpływ na urządzenia i aplikacje, a następnie automatycznie reagować na wykryte problemy.

System łączy trzy główne elementy:
Network Experimentation czyli tworzenie kontrolowanych warunków sieciowych, Network Monitoring - obserwowanie parametrów sieci i Automated Response -  automatyczne wykonywanie działań na podstawie warunków.



Przykład zamierzonego scenariusza dzialania::

> Administrator ustawia 150 ms opóźnienia i 5% utraty pakietów dla jednego urządzenia. NREPlatform rozpoczyna eksperyment, zbiera dane, wykrywa pogorszenie jakości połączenia i wykonuje zdefiniowaną wcześniej akcję.


2. Główna architektura

NREPlatform może składać się z kilku modułów:

NREPlatform
                      |
       +--------------+--------------+
       |              |              |
 Experiment Engine  Monitoring    Response Engine
       |              |              |
       +--------------+--------------+
                      |
                Policy Engine
                      |
                OpenWrt / Linux
                      |
        +-------------+-------------+
        |             |             |
       tc           nftables       DNS
    netem          firewall       services

Na OpenWrt głównym backendem może być daemon napisany w C++ natomiast interfejs można zintegrować z LuCI na ruterze.


3. Experiment Engine ( podstawowy moduł odpowiedzialny za tworzenie eksperymentów sieciowych.))

Administrator wybiera:
urządzenie,
interfejs,
kierunek ruchu,
czas trwania,
parametry eksperymentu.

Na przyklad:
Administrator wybiera:

Device: 192.168.1.50
Latency: 100 ms
Packet loss: 3%
Duration: 60 seconds

Po kliknięciu Start Experiment system konfiguruje mechanizmy Linux Traffic Control i rozpoczyna eksperyment.

Po 60 sekundach konfiguracja zostaje automatycznie przywrócona.

4. Latency Injection ((pozwala sztucznie zwiększyć opóźnienie połączenia)

Przykład
Latency: +150 ms

Komputer normalnie:
ping: 10 ms

Podczas eksperymentu:
ping: ~160 ms


5. Zamierzane zastosowanie: 
można sprawdzić, jak aplikacja zachowuje się przy:

słabym połączeniu,
dużej odległości od serwera,
przeciążonej sieci,
wysokim RTT.


6. Packet Loss Simulation (pozwala symulować utratę pakietów)

Przykład
Packet Loss: 5%
 -- system powoduje, że część pakietów zostaje utracona.

Można wtedy sprawdzić np.:
ping:
10 ms
11 ms
timeout
10 ms
timeout
12 ms

Zastosowanie
Testowanie odporności:
aplikacji internetowych,
gier,
VoIP,
VPN,
własnych API.


6. Bandwidth Limiting (pozwala ograniczyć przepustowość konkretnego urządzenia)

Przykład
Komputer normalnie:
Download: 300 Mb/s

Eksperyment:
Download limit: 5 Mb/s

NREPlatform nakłada limit tylko na wybrane urządzenie.

Można dzięki temu sprawdzić, czy aplikacja poprawnie działa przy bardzo ograniczonym łączu.


7. Jitter Simulation (jjitter oznacza zmienność opóźnienia).

Zamiast:

100 ms
100 ms
100 ms
100 ms

platforma może stworzyć warunki:

70 ms
130 ms
95 ms
180 ms
110 ms

Zastosowanie
Szczególnie przydatne przy testowaniu:
rozmów głosowych,
wdeokonferencji,
steamingu,
gier sieciowych.

8. DNS Failure Simulation (NREPlatform może kontrolowanie symulować problemy z DNS)

Przykład
Eksperyment:
DNS Failure
Duration: 30 s
Target: 192.168.1.50

Podczas eksperymentu urządzenie nie może poprawnie rozwiązywać nazw:

example.com ->> DNS failure

Jednocześnie można sprawdzić, czy aplikacja potrafi:
użyć cache,
ponowić zapytanie,
wykorzystać alternatywny resolver,
poprawnie poinformować użytkownika.

9. Connection Interruption (system może zasymulować chwilową utratę połączenii).

Przykład
Online
Online
Online
OFFLINE
OFFLINE
OFFLINE
Online
Online

Na przykład:
Duration: 10 seconds
Po zakończeniu eksperymentu połączenie zostaje automatycznie przywrócone.

10. Target Selection
- możliwość określenia, kogo eksperyment ma dotyczyć.

Możliwe kryteria:
IP address
MAC address
IPv4 subnet
IPv6 address
TCP port
UDP port
Interface

Przykład
Target:
MAC = AA:BB:CC:DD:EE:FF
Eksperyment dotyczy tylko tego urządzenia.
Pozostałe urządzenia w sieci działają normalie.


11. Traffic Direction - system może rozróżniać kierunek ruchu.

Download
Internet ->> Device

Upload
Device ->> Internet

Można ustawić:
Download: 5 Mb/s
Upload: unlimited

- testwoaniw zachowania aplikacji

12. Experiment Profiles zapisywanie gotowe konfiguraci

Przykład:
Profile: Poor Mobile Network

Latency: 100 ms
Jitter: 30 ms
Packet Loss: 2%
Bandwidth: 10 Mb/s
Duration: 120 s

Potem wystarczy:
Start Profile
zamiast konfigurowania wszystkiego od początku.

13. Network Monitoring - podczas eksperymentu system zbiera dane.

Przykładowe metryki:
Latency
Packet loss
Jitter
Bandwidth
Packets transmitted
Packets dropped
DNS response time
Connection state

Przykład
Experiment #24

Latency       137 ms
Packet loss   4.2%
Jitter        31 ms
Download      8.4 Mb/s
Packets       18452
Dropped       774


14. Real-Time Monitoring
Dane mogą być wyświetlane na żywo.
Przykładowo:
Latency
200 ms |                    *
150 ms |             *  *  *
100 ms |       *  * 
 50 ms | *  *
       +---------------------
          0   10   20   30 s

15. Network Health Score
System może obliczać syntetyczny stan sieci na podstawie kilku parametrów.
Przykładowe dane:
Latency:       30 ms
Packet loss:   0.2%
Jitter:        4 ms
Bandwidth:     80 Mb/s

System może określić stan:
HEALTHY

Przy:
Latency:       180 ms
Packet loss:   8%
Jitter:        70 ms
stan może przejść na:
DEGRADED

16. Policy Engine
Pozwala tworzyć reguły:

IF condition
THEN action

Przykład
IF latency > 100ms
THEN enable_qos

jesli opóźnienie przekroczy 100 ms, włącz określoną politcy QoS


17. Multiple Conditions
- reguły mogą mieć więcej warunków.

Przykład:
IF
    latency > 100ms
    AND
    packet_loss > 3%
THEN
    activate_policy "degraded_network"

System musi spełnić oba warunki.

18. OR Conditions

IF
    latency > 150ms
    OR
    packet_loss > 5%
THEN
    trigger_alert

Wystarczy jeden z warunków.

19. Automatic Response Engine

 po wykryciu problemu system może automatycznie wykonać akcję.

Przykłady akcji:
Apply QoS
Change DNS
Block traffic
Allow traffic
Restart service
Change network profile
Start experiment
Stop experiment
Generate alert
Create log entry


20. Automated Experiment

Najciekawszy scenariusz:
systwm sam wykonuje scenariusz po po okreslonym wrunku
Przykład
IF network_quality < threshold
THEN start experiment "connection_test"


21. Safe Rollback
Każdy eksperyment powinien mieć mechanizm automatycznego cofnięcia zmian.

Przykład:
Before:
Bandwidth = unlimited
Experiment:
Bandwidth = 5 Mb/s
After:
Bandwidth = unlimited

Jeżeli daemon zostanie zamknięty lub eksperyment się zakończy, system przywraca konfig.

22. Experiment Scheduler
Możliwość zaplanowania eksperymentu.
Przykład:
Experiment:
Poor Network

Start:
18:00

Duration:
5 minutes

System automatycznie rozpocznie i zakończy test.

23. Experiment Chain
Można wykonywać kilka eksperymentów jeden po drugim.

Przykład:
Step 1:
Latency +50 ms
30 seconds

Step 2:
Latency +100 ms
30 seconds

Step 3:
Packet loss 5%
30 seconds

Step 4:
Bandwidth 5 Mb/s
30 seconds

Powstaje z tego cały scenariusz testowy.

24. Failure Scenarios

Na przykład:
Weak Wi-Fi
Latency: 80 ms
Jitter: 25 ms
Loss: 2%

Unstable Connection
Latency: 150 ms
Jitter: 80 ms
Loss: 5%

Congested Network
Bandwidth: 2 Mb/s

DNS Failure

DNS unavailable


25. Logging

Każde działanie jest zapisywane.

Przykład:

2026-09-18 11:32:01
Experiment started
Target: 192.168.1.50

2026-09-18 11:32:02
Latency injection: 100ms

2026-09-18 11:32:03
Packet loss: 3%

2026-09-18 11:33:01
Experiment finished

2026-09-18 11:33:02
Configuration restored


26. Experiment History

System przechowuje historię:
Experiment #001
Experiment #002
Experiment #003
...

Przy każdym:
Start time
Duration
Target
Parameters
Results
Status

27. Export Results

Wyniki można eksportować np. jako:
JSON
CSV

Przykład:

{
  "latency_avg": 124,
  "packet_loss": 3.2,
  "duration": 60
}

Można później analizować wyniki na komputerze.

28. LuCI Integration

NREPlatform może posiadać własną sekcję w panelu OpenWrt LuCI.

Przykładowa struktura:
NREPlatform >
Dashboard
Experiments
Profiles
Policies
Monitoring
History
Settings

nie trzeba wpisywac komend w termianlu.

29. Dashboard
Dashboard pokazuje aktualny stan:
Network Status: DEGRADED

Latency:       126 ms
Packet Loss:   3.1%
Jitter:        27 ms
Bandwidth:     8.2 Mb/s

Active Experiment:
Poor Network

Time Remaining:
00:42
// to byl przykald

30. API
NREPlatform posiada własne REST API.

Przykładowo:
POST /api/experiments
z konfiguracją eksperymentu.
Można wtedy sterować routerem z:
własnej aplikacji,
skryptu,
serwera,
CI/CD,
innego systemu administracyjnego.

to mozna bedzie skonfigurowac ale nie bedzie obowiązkowo.
31. CLI
Oprócz GUI można stworzyć CLI:
nre experiment start poor-network

albo:
nre experiment list

oraz:
nre status

32. Permission System
Można dodać poziomy dostęp:u
Viewer
Operator
Administrator

33. Audit Log
Oprócz zwykłych logów można prowadzić historię zmian administracyjnych.

Przykład:
User: admin
Action: created policy
Policy: high_latency_response
Time: 11:43

34. Resource Protection
Platforma powinna pilnować, żeby eksperyment nie przeciążył routera.

Przykład:
Maximum experiment duration: 10 min
Maximum concurrent experiments: 3

Można również blokować niebezpieczne kombinacje konfiguracji.
35. Experiment Isolation
Eksperyment powinien być możliwie dokładnie ograniczony do określonego celu.
 naprzyklad:
Target:
192.168.1.50

Effect:
Latency +100 ms

Pozostałe urządzenia:
192.168.1.20 → normal
192.168.1.30 → normal
192.168.1.40 → normal
192.168.1.50 → experiment

36. Preset Marketplace / Preset Repository

W dalszym etapie można stworzyć możliwość importowania gotowych scenariuszy.

Przykład:
gaming.json
voip.json
mobile-network.json
satellite-network.json
unstable-wifi.json

Każdy preset definiuje parametry eksperymentu.

37. Test Automation

NREPlatform może być wykorzystywany do automatycznego testowania aplikacji.

Przykład:
1. Start application
2. Start network experiment
3. Wait 60 seconds
4. Collect metrics
5. Stop experiment
6. Restore network
7. Export results

To pozwala wykorzystać platformę jako element testów aplikacji sieciowych.

38. Przykładowy pełny scenariusz

Załóżmy, że masz komputer:
192.168.1.50

Administrator tworzy eksperyment:
Name:
Unstable Connection

Latency:
100 ms

Jitter:
30 ms

Packet Loss:
3%

Bandwidth:
10 Mb/s

Duration:
120 seconds

NREPlatform:

1. Zapamiętuje aktualną konfigurację.
2. Wybiera urządzenie 192.168.1.50.
3. Nakłada parametry eksperymentu.
4. Rozpoczyna zbieranie metryk.
5. Monitoruje połączenie.
6. Zapisuje wyniki.
7. Po 120 sekundach usuwa ograniczenia.
8. Przywraca poprzednią konfigurację.
9. Zapisuje raport.

Następnie można dodać politykę:

IF
    packet_loss > 5%
    AND
    latency > 150ms

THEN
    trigger_alert


39. calksc

Przykładowy stack:
OS:
OpenWrt

Backend:
C++
Network control:
Linux tc
tc netem
nftables

Front:
LuCI
JavaScript

API:
REST

Data:
JSON / SQLite( toggle)

Logging:
systemd/logread + własne logi
