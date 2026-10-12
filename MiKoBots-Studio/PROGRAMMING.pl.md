# Programowanie ramienia ARM w MiKoBots Studio

Pole programu ma trzy rodzaje programów, wybierane z listy nad nim: **Python**, **Blockly**
oraz **Text (Kawasaki AS / G-code)**. Python i Text zachowują swój tekst przy przełączaniu.
Zielony przycisk **Run** obok listy uruchamia program na robocie albo na modelu 3D, zależnie od
przełącznika **ROBOT / SIMULATION** w sekcji *Mode*; podczas działania programu przycisk robi
się czerwony i zatrzymuje program.

## Ramię ARM

| Część | Kanał PCA9685 | Zakres | Polecenie |
|---|---|---|---|
| J1 podstawa | 0 | -90 … 90° | ruchy przegubów i ruchy do pozycji |
| J2 bark | 1 | -60 … 95° | ruchy przegubów i ruchy do pozycji |
| J3 łokieć | 2 | -90 … 60° | ruchy przegubów i ruchy do pozycji |
| Obrót nadgarstka (roll) | 3 | -90 … 90°, 0 = chwytak prosto | `WristRoll` |
| Chwytak (narzędzie) | 4 | 0 … 100 % | `tool.MoveTo` (narzędzie jeszcze niezamontowane) |
| Pochylenie nadgarstka (pitch) | 5 | -90 … 90° względem poziomu, 0 = poziomo, + = w górę | `WristPitch` |

- Pozycje podaje się w **mm**: X do przodu, Y w lewo, Z w górę, liczone od podstawy.
- Prędkość `v` i przyspieszenie `a` podaje się w **%** maksimum robota (1–100).
- ARM ma 3 osie, więc każda lista pozycji lub kątów przegubów ma **3 wartości**.
- Chwytak zachowuje swoje pochylenie, gdy ramię się porusza. `MoveJ` / `MoveL` uwzględniają
  pochylenie, więc X Y Z to zawsze końcówka chwytaka. Obrót (roll) nie zmienia pozycji.

## Python

Program w Pythonie zaczyna się od potrzebnych importów:

```python
from robot_library import Move, Tool, IO
import time

robot = Move()
tool = Tool()
IO = IO()
```

### Ruch robota: `robot = Move()`

| Polecenie | Działanie |
|---|---|
| `robot.Home()` | Bazowanie robota: przeguby do pozycji bazowej, roll 0, pitch 0. **Wymagane raz po podłączeniu, przed `MoveJ`, `MoveL`, `OffsetJ`, `OffsetL`.** |
| `robot.MoveJ([x, y, z], v, a)` | Przesuwa końcówkę chwytaka do X Y Z; każdy przegub porusza się po swojej drodze. Zawsze podawaj `v` i `a`. |
| `robot.MoveL([x, y, z], v=50, a=50)` | Przesuwa końcówkę chwytaka do X Y Z po linii prostej. |
| `robot.OffsetJ([dx, dy, dz], v=50, a=50)` | Przesuwa końcówkę o dx dy dz względem obecnej pozycji (ruch przegubowy). |
| `robot.OffsetL([dx, dy, dz], v=50, a=50)` | Przesuwa końcówkę o dx dy dz po linii prostej. |
| `robot.MoveJointPos([j1, j2, j3], v=50, a=50)` | Ustawia przeguby na podane kąty (w stopniach). Nie wymaga `Home()`. |
| `robot.JogJoint([dj1, dj2, dj3], v, a)` | Obraca każdy przegub o podaną liczbę stopni. Bez `v`/`a` używa prędkości ręcznego sterowania z panelu. |
| `robot.WristRoll(deg, v=50, a=50)` | Obraca chwytak wokół własnej osi, -90 … 90°. |
| `robot.WristPitch(deg, v=50, a=50)` | Pochyla chwytak, -90 … 90° względem poziomu (0 = poziomo, + = w górę). |

Ruch poza zasięgiem albo poza zakresem przegubu jest odrzucany; dziennik (log) pokazuje powód.

### Narzędzie: `tool = Tool()`

| Polecenie | Działanie |
|---|---|
| `tool.SetTool("gripper")` | Wybiera narzędzie po jego nazwie z ustawień robota. Wywołaj raz przed innymi poleceniami narzędzia. |
| `tool.MoveTo(pos)` | Narzędzie z serwem (chwytak): 0 = zamknięty … 100 = otwarty. |
| `tool.State("HIGH")` / `tool.State("LOW")` | Narzędzie na przekaźniku (przyssawka, magnes): włącz / wyłącz. |

### Wejścia i wyjścia: `IO = IO()`

| Polecenie | Działanie |
|---|---|
| `IO.SetIO(n, "OUTPUT")` / `IO.SetIO(n, "INPUT")` | Ustawia IO numer `n` jako wyjście albo wejście (pin jest zdefiniowany w ustawieniach robota). |
| `IO.digitalWrite(n, "HIGH")` / `IO.digitalWrite(n, "LOW")` | Włącza / wyłącza wyjście `n`. |
| `IO.digitalRead(n)` | `True`, gdy wejście `n` jest włączone. |

### Czekanie i zwykły Python

`time.sleep(sekundy)` czeka. Wszystko inne w Pythonie działa normalnie: pętle `for` i `while`,
`if`, zmienne, funkcje, `print()` (wynik pojawia się w dzienniku).

**Czekanie między ruchami:**
- **USB:** każde polecenie ruchu kończy się dopiero, gdy robot skończy ruch, więc
  `time.sleep(1)` po ruchu to rzeczywista przerwa 1 s.
- **Bluetooth:** polecenie ruchu kończy się zaraz po wysłaniu, więc czekanie biegnie, gdy ramię
  jeszcze się porusza. Czas czekania musi być dłuższy niż ruch.

### Przykład

```python
from robot_library import Move
import time

robot = Move()
robot.Home()

robot.MoveJ([200, 0, 250], 40, 40)      # nad miejscem pobrania
robot.WristPitch(-30, 40, 40)           # chwytak w dół
robot.OffsetL([0, 0, -50], 20, 20)      # 50 mm w dół po linii prostej
time.sleep(1)
robot.OffsetL([0, 0, 50], 20, 20)       # z powrotem w górę
robot.WristPitch(0, 40, 40)             # znowu poziomo
robot.WristRoll(90, 40, 40)             # obrót chwytaka
robot.MoveJ([150, 100, 250], 40, 40)    # do miejsca odłożenia
```

Po uruchomieniu programu w polu programu jest test serw, który porusza każdym serwem osobno.

## Blockly

ARM używa bloków z kategorii **Robot 3 axis**. Bloki dla 5 i 6 osi na nim nie działają.

| Kategoria | Bloki | Uruchamiany kod Python |
|---|---|---|
| Robot 3 axis | MoveJ, MoveL, OffsetJ, OffsetL (X Y Z v a) | `robot.MoveJ(...)` itd. |
| | Wrist roll (deg v a) | `robot.WristRoll(...)` |
| | Wrist pitch (deg v a) | `robot.WristPitch(...)` |
| Tool | set tool, tool move to, tool state | `tool.SetTool / MoveTo / State` |
| I/O | io pin, set io pin, digital write, digital read | `IO.SetIO / digitalWrite / digitalRead` |
| Wait | Wait … s | `time.sleep(...)` |
| Loops, Logic, Math, Text, Lists, Variables, Functions | standardowe bloki Blockly | zwykły Python |

Blockly nie ma bloku dla `Home`, `MoveJointPos` ani `JogJoint`: przed uruchomieniem programu
Blockly z MoveJ / MoveL / Offset wykonaj bazowanie przyciskiem *Home*.

## Programy tekstowe (Kawasaki AS / G-code)

Wybierz **Text (Kawasaki AS / G-code)** z listy i wpisz program jako zwykły tekst. Aplikacja
najpierw sprawdza cały program: jeśli któraś linia ma błąd, dziennik pokazuje wszystkie błędy
z numerami linii i robot się nie porusza. Podczas wykonywania dziennik pokazuje każdą linię.

- Linie Kawasaki AS i G-code można mieszać; każda linia jest czytana osobno. Wielkość liter
  nie ma znaczenia.
- Komentarze: `;` do końca linii oraz `( … )` w liniach G-code.
- Jednostki: mm, stopnie, sekundy; prędkość i przyspieszenie w %.
- Programy tekstowe poruszają tylko J1–J3 oraz X Y Z; obrót (roll) i pochylenie (pitch)
  nadgarstka zostają bez zmian.
- Przed ruchami do pozycji wykonaj bazowanie (`HOME` / `G28` albo przycisk *Home*), tak jak
  w Pythonie.

### Kawasaki AS

| Polecenie | Działanie |
|---|---|
| `HOME` | bazowanie robota (w symulacji: przeguby na 0) |
| `JMOVE poza` | ruch do pozy, każdy przegub po swojej drodze |
| `LMOVE poza` | ruch do pozy kartezjańskiej po linii prostej |
| `DRAW dx, dy, dz` | ruch o dx dy dz od obecnej pozycji narzędzia, po linii prostej |
| `SPEED n` (`SPEED n ALWAYS`) | prędkość w % dla kolejnych ruchów (domyślnie 50) |
| `ACCEL n` / `DECEL n` | przyspieszenie w % dla kolejnych ruchów (jedna wartość dla obu) |
| `TWAIT t` | czekaj t sekund |
| `OPENI` / `CLOSEI` (albo `OPEN` / `CLOSE`) | otwórz / zamknij chwytak |
| `SIGNAL n` / `SIGNAL -n` | włącz / wyłącz wyjście n |
| `POINT nazwa = TRANS(x, y, z)` | nazwana poza kartezjańska |
| `POINT #nazwa = #PPOINT(j1, j2, j3)` | nazwana poza przegubowa (kąty w stopniach) |
| `.PROGRAM nazwa()` … `.END` | ramka programu, można ją pominąć |

Poza to `TRANS(x, y, z)`, `#PPOINT(j1, j2, j3)` albo nazwa zdefiniowana przez `POINT`.
`TRANS(x, y, z, o, a, t)` jest akceptowane, ale O A T są pomijane (ostrzeżenie w dzienniku).
Nieobsługiwane: `IF`, `FOR`, `WHILE`, `GOTO`, etykiety, `SWAIT`, `TDRAW`, `HERE`.

### G-code

| Kod | Działanie |
|---|---|
| `G0 X Y Z` | ruch, każdy przegub po swojej drodze |
| `G1 X Y Z F` | ruch po linii prostej |
| `G4 P<ms>` / `G4 S<s>` | czekanie |
| `G28` | bazowanie |
| `G90` / `G91` | współrzędne bezwzględne (domyślnie) / względne dla kolejnych ruchów |
| `G21` | milimetry (domyślnie) |
| `F<mm/min>` | prędkość: 9000 mm/min = 100 % (150 mm/s); obowiązuje dla kolejnych ruchów |
| `M3` / `M5` | zamknij / otwórz chwytak |
| `N…` | numer linii, pomijany |

Osie pominięte w linii zachowują swoją wartość: `G1 Z50` zmienia tylko Z. Nieobsługiwane: cale
(`G20`), łuki (`G2`, `G3`) i wszystkie inne kody.

### Przykład

```
; Kawasaki AS
HOME
SPEED 30
POINT pick = TRANS(200, 0, 250)
JMOVE pick
DRAW 0, 0, -40
TWAIT 1
DRAW 0, 0, 40
; G-code
G90
G1 X150 Y100 Z250 F3000
G4 P500
G28
```

## Panel sterowania

- **Jog Joint:** `-` / `+` dla J1–J3, **Roll** i **Pitch**, o wartość kroku (jog distance).
- **Jog Axis:** przesuwa końcówkę wzdłuż X, Y, Z (wymaga Home).
- **Tool:** otwiera / zamyka chwytak.
- **Mode:** duży przełącznik **ROBOT / SIMULATION** wybiera, gdzie działają programy i ręczne
  sterowanie. Na starcie jest SIMULATION, po podłączeniu robota przełącza się na ROBOT, a po
  rozłączeniu z powrotem; ręczne przełączenie na ROBOT bez robota wymaga potwierdzenia.
- **Robot:** *Connect robot*, *Home*, **Pause / Resume** (zatrzymuje robota w trakcie ruchu
  i wznawia ruch) oraz czerwony **STOP** (od razu kończy program i ruch; ręczne sterowanie,
  Home i Run działają zaraz po nim).

## Polecenia firmware

To, co aplikacja wysyła do robota; przydatne przy testach z monitorem portu szeregowego
(57600 bodów, jedno polecenie w linii, robot odpowiada na każde polecenie `END`):

| Polecenie | Znaczenie |
|---|---|
| `CONNECT` | odpowiada `ROBOT_CONNECTED` (oraz raportem I2C, jeśli nie znaleziono sterownika serw) |
| `Home` | bazowanie wszystkich przegubów, roll i pitch |
| `MoveJoint A<j1>B<j2>C<j3>D<v>E<a>` | kąty przegubów |
| `jogJ A<dj1>B<dj2>C<dj3>D<v>E<a>` | względne kąty przegubów |
| `MoveJ A<x>B<y>C<z>D<v>E<a>` / `MoveL …` | pozycja, ruch przegubowy / po linii prostej |
| `OffsetJ …` / `OffsetL …` | pozycja względna |
| `WristRoll A<deg>B<v>C<a>` | obrót nadgarstka, odpowiada `ROLL: <kąt>` |
| `WristPitch A<deg>B<v>C<a>` | pochylenie nadgarstka, odpowiada `PITCH: <kąt>` i nową pozycją |
| `Tool_move_to pos(<0-100>)` | chwytak |
| `stop` / `pauze` / `play` | zatrzymanie, pauza, wznowienie bieżącego ruchu |
