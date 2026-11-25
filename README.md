# aliense44 Bruce Firmware Fork

This is a personal fork of [Bruce](https://github.com/pr3y/Bruce), focused on RF experimentation, ProtoPirate improvements, and practical Sub-GHz workflow enhancements.

The upstream Bruce project remains the primary source of the firmware core, device support, and general features. This fork is used to develop, test, and document selected improvements before proposing them upstream.

## Focus

This fork focuses on:

- ProtoPirate RF decoding and emulation
- Sub-GHz protocol workflows
- KeeLoq rolling-code handling
- Practical RF file management
- NRF24 spectrum analysis and jamming workflow
- Improvements that can later be proposed to the upstream Bruce project

## Custom startup branding and version

The boot screen displays `Bruce & Aliense44` instead of the default `Bruce` name.

The displayed firmware version is manually set in `platformio.ini`:

```text
a8fa5f7a - 23.11.2026
```

This makes it easy to identify the firmware build currently installed on the device.

## Submenu layout

Submenus use a compact, unframed carousel inspired by Bruce 1.16 instead of the boxed vertical cards. The selected option is centered, with the previous and next options shown above and below it. Navigation wraps around at the ends of the list.

The selected option uses the primary color and an underline. A slim indicator on the right shows the current position in the submenu.

Long labels stay within the screen width. The selected label is reduced in size and wrapped at a word boundary across two lines where possible; if it still cannot fit, it is shortened with an ellipsis. Neighboring labels use a smaller font and are shortened with an ellipsis when needed. The submenu title and status bar remain visible.
## NRF24 tools

This fork improves the NRF24 spectrum, active-channel reporting, and jamming workflow.

### NRF Spectrum

NRF Spectrum scans all 128 NRF24 channels, from channel 0 to channel 127.

It displays a live spectrum view with:

- Current channel activity
- Peak-hold indication
- The strongest currently detected channel
- A graphical spectrum plot

The spectrum is refreshed approximately every 40 ms, while a new plot row is added approximately every 120 ms.

### NRF Spectrum Active Report

The **Active** view collects channels that showed activity during the current session.

It provides:

- A session-specific list of active NRF24 channels
- Channel number and approximate frequency
- A human-readable approximate range hint, such as BLE, Wi-Fi, Bluetooth, USB, video, RC, or Zigbee
- Paging through detected channels
- A session elapsed-time counter
- Saving the report to the microSD card

The report is saved as:

```text
BruceNRF/scan_NNNN.csv
```

Saving a report does not clear the channels collected during the session. The active-channel list is reset only when the Active view is closed and opened again.

The CSV uses version 5 and contains these columns:

```text
formatversion,nrfchannel,frequencymhz,sessionelapsedms,scanminchannel,scanmaxchannel,rangehint,categorymask
```

`range_hint` describes only the approximate frequency context. It is not protocol identification.

### NRF Jammer

The NRF Jammer menu includes:

- Start
- Load Scan File
- Limit
- Back

The safety timeout can be selected from:

- 30 seconds
- 1 minute
- 2 minutes
- 3 minutes
- 5 minutes

The `formatNrfTimeout()` function displays the remaining time in a readable form, for example `30 s`, `1 min`, or `1 min 30 s`.

While the jammer is running, the screen shows `TIME : ... (10 s refresh)`. The countdown display refreshes every 10 seconds; during the final 5 seconds, it refreshes every second.

### Jammer channel sources

The jammer can use either its built-in channel lists or channels loaded from a CSV report created by NRF Spectrum Active Report.

CSV files are selected from:

```text
BruceNRF
```

The loader reads NRF24 channels from the CSV, validates that they are within the 1-127 channel range and 2.400-2.500 GHz frequency range, and reads the optional `categorymask` column.

When a scan file is loaded, the jammer automatically uses random channel hopping.

### Jammer channel categories

The jammer can filter loaded channels by category:

- Test
- Wi-Fi
- BLE
- BLE Adv Priority
- Bluetooth
- USB
- Video Stream
- RC
- Zigbee
- Full

The category filter uses the `categorymask` value stored in the CSV report.

### Jammer hopping

The jammer supports:

- Sequential hopping
- Random hopping

The selected mode and hopping behaviour can be changed while the jammer is running.

## ProtoPirate Remote

One of the additions in this fork is **ProtoPirate Remote**, a quick workflow for managing decoded KeeLoq remotes.

It allows selecting a previously decoded `.sub` file from:

```text
BruceRF/ProtoPirate
```

and transmitting regenerated KeeLoq rolling-code frames without repeatedly going through `Sub Decode` and `Emulate / encode`.

### KeeLoq Remote features

- Select a decoded KeeLoq `.sub` file
- Show the current counter, button code, and counter step
- Transmit the next regenerated KeeLoq frame
- Preview the updated counter, `Key`, and `Hop`
- Select a counter step: 1, 2, 3, 5, or 10
- Skip counter values without transmitting
- Change the KeeLoq button/command code
- Reset the counter to the value stored in the original `.sub` file

### Persistent state

The original `.sub` file is never modified.

For each source file, ProtoPirate creates separate files:

```text
BruceRF/ProtoPirate/<name>.sub
BruceRF/ProtoPirate/<name>.state
BruceRF/ProtoPirate/<name>.log
```

- `.sub` — original decoded capture
- `.state` — current counter, step, and button code
- `.log` — history of transmitted counters

After restarting the device, the next transmission continues from the counter stored in the `.state` file.

## KeeLoq scope

KeeLoq rolling-code regeneration is supported only when:

- The decoded protocol is `KeeLoq`
- The manufacturer is recognized
- A serial number is available

Unknown KeeLoq manufacturers remain in replay mode, because reliable rolling-code regeneration cannot be guaranteed.

Other RF protocols continue using their existing decode, encode, and emulation paths.

## Tested example

The KeeLoq Remote workflow was tested with a DTM_Neo remote.

Three frames generated by ProtoPirate matched the next three frames produced by the physical remote:

- Counter values matched
- Generated `Key` values matched
- Generated `Hop` values matched
- Serial, button code, and fixed portion remained consistent

## Upstream project

This fork is based on Bruce:

https://github.com/pr3y/Bruce

For general Bruce documentation, supported hardware, installation instructions, and the full feature list, see the upstream project:

https://github.com/pr3y/Bruce  
https://wiki.bruce.computer/

## Disclaimer

This fork is intended for legal and authorized security research, education, and testing only. Do not use it to interfere with systems you do not own or do not have explicit permission to test.

The upstream Bruce project is distributed under the Affero General Public License (AGPL). This fork follows the same licensing requirements.

---

# aliense44 — Fork firmware Bruce

To osobisty fork projektu [Bruce](https://github.com/pr3y/Bruce), skoncentrowany na eksperymentach RF, ulepszeniach ProtoPirate oraz praktycznych usprawnieniach pracy z Sub-GHz.

Nadrzędny projekt Bruce pozostaje głównym źródłem rdzenia firmware, obsługi urządzeń i ogólnych funkcji. Ten fork służy do tworzenia, testowania i dokumentowania wybranych ulepszeń przed ewentualnym zaproponowaniem ich upstreamowi.

## Cel forka

Ten fork koncentruje się na:

- dekodowaniu i emulacji RF w ProtoPirate
- workflow dla protokołów Sub-GHz
- obsłudze rolling code KeeLoq
- praktycznym zarządzaniu plikami RF
- analizie widma, raportowaniu aktywnych kanałów i workflow jammingu NRF24
- ulepszeniach, które później mogą zostać zaproponowane upstreamowi Bruce

## Niestandardowy ekran startowy i wersja

Ekran startowy wyświetla nazwę `Bruce & Aliense44` zamiast domyślnego `Bruce`.

Wyświetlana wersja firmware jest ustawiana ręcznie w pliku `platformio.ini`:

```text
a8fa5f7a - 23.11.2026
```

Dzięki temu łatwo rozpoznać, która wersja firmware jest aktualnie zainstalowana na urządzeniu.

## Układ podmenu

Podmenu korzysta teraz z kompaktowej karuzeli bez kart i ramek, inspirowanej układem Bruce 1.16. Wybrana pozycja jest wyświetlana pośrodku, a poprzednia i następna pojawiają się nad nią i pod nią. Nawigacja zawija się na końcach listy.

Wybrana pozycja jest wyróżniona kolorem głównym i podkreśleniem. Wąski wskaźnik po prawej pokazuje położenie w podmenu.

Długie nazwy mieszczą się w szerokości ekranu. Nazwa wybranej pozycji jest zmniejszana i, jeśli to możliwe, zawijana na granicy słów do dwóch wierszy; jeśli nadal się nie mieści, zostaje skrócona wielokropkiem. Sąsiednie pozycje używają mniejszej czcionki i w razie potrzeby również są skracane wielokropkiem. Tytuł podmenu i pasek statusu pozostają widoczne.
## Narzędzia NRF24

Ten fork rozwija workflow analizy widma, raportowania aktywnych kanałów oraz jammingu NRF24.

### NRF Spectrum

NRF Spectrum skanuje wszystkie 128 kanałów NRF24, od kanału 0 do 127.

Wyświetla żywy widok widma z:

- aktualną aktywnością kanałów
- wskazaniem szczytów
- aktualnie najsilniejszym wykrytym kanałem
- graficznym wykresem widma

Widmo jest odświeżane około co 40 ms, a nowy wiersz wykresu jest dodawany około co 120 ms.

### NRF Spectrum Active Report

Widok **Active** zbiera kanały, na których wykryto aktywność podczas bieżącej sesji.

Oferuje:

- listę aktywnych kanałów NRF24 zebranych w sesji
- numer kanału i przybliżoną częstotliwość
- czytelny, przybliżony opis zakresu, np. BLE, Wi‑Fi, Bluetooth, USB, wideo, RC lub Zigbee
- stronicowanie wykrytych kanałów
- licznik czasu trwania sesji
- zapis raportu na kartę microSD

Raport jest zapisywany jako:

```text
BruceNRF/scan_NNNN.csv
```

Zapis raportu nie czyści kanałów zebranych podczas sesji. Lista aktywnych kanałów jest resetowana dopiero po zamknięciu i ponownym otwarciu widoku Active.

Plik CSV używa wersji 5 i zawiera kolumny:

```text
formatversion,nrfchannel,frequencymhz,sessionelapsedms,scanminchannel,scanmaxchannel,rangehint,categorymask
```

`range_hint` opisuje wyłącznie przybliżony kontekst częstotliwości. Nie jest identyfikacją protokołu.

### NRF Jammer

Menu NRF Jammer zawiera:

- Start
- Load Scan File
- Limit
- Back

Limit bezpieczeństwa można wybrać z następujących wartości:

- 30 sekund
- 1 minuta
- 2 minuty
- 3 minuty
- 5 minut

Funkcja `formatNrfTimeout()` wyświetla pozostały czas w czytelnej postaci, np. `30 s`, `1 min` albo `1 min 30 s`.

Podczas działania jammera na ekranie wyświetla się `TIME : ... (10 s refresh)`. Licznik odświeża ekran co 10 sekund; w ostatnich 5 sekundach odświeża go co sekundę.

### Źródła kanałów jammera

Jammer może korzystać z wbudowanych list kanałów albo z kanałów załadowanych z raportu CSV utworzonego przez NRF Spectrum Active Report.

Pliki CSV wybiera się z folderu:

```text
BruceNRF
```

Loader odczytuje kanały NRF24 z pliku CSV, sprawdza, czy mieszczą się w zakresie 1-127 oraz 2,400-2,500 GHz, i odczytuje opcjonalną kolumnę `categorymask`.

Po załadowaniu pliku skanu jammer automatycznie używa losowego przeskakiwania kanałów.

### Kategorie kanałów jammera

Jammer może filtrować załadowane kanały według kategorii:

- Test
- Wi‑Fi
- BLE
- BLE Adv Priority
- Bluetooth
- USB
- Video Stream
- RC
- Zigbee
- Full

Filtr kategorii korzysta z wartości `categorymask` zapisanej w raporcie CSV.

### Hopping jammera

Jammer obsługuje:

- przeskok sekwencyjny
- przeskok losowy

Wybrany tryb i sposób przeskoków można zmieniać podczas działania jammera.

## ProtoPirate Remote

Jednym z dodatków w tym forku jest **ProtoPirate Remote** — szybki workflow do obsługi zdekodowanych pilotów KeeLoq.

Pozwala wybrać wcześniej zdekodowany plik `.sub` z folderu:

```text
BruceRF/ProtoPirate
```

i nadawać regenerowane ramki KeeLoq rolling code bez wielokrotnego przechodzenia przez `Sub Decode` oraz `Emulate / encode`.

### Funkcje KeeLoq Remote

- Wybór zdekodowanego pliku `.sub` KeeLoq
- Wyświetlanie aktualnego licznika, kodu przycisku i kroku licznika
- Nadawanie kolejnej, regenerowanej ramki KeeLoq
- Podgląd zaktualizowanego licznika, `Key` i `Hop`
- Wybór kroku licznika: 1, 2, 3, 5 lub 10
- Przeskakiwanie wartości licznika bez nadawania
- Zmiana kodu przycisku KeeLoq
- Przywrócenie licznika z oryginalnego pliku `.sub`

### Trwały stan

Oryginalny plik `.sub` nie jest modyfikowany.

Dla każdego pliku źródłowego ProtoPirate tworzy osobne pliki:

```text
BruceRF/ProtoPirate/<nazwa>.sub
BruceRF/ProtoPirate/<nazwa>.state
BruceRF/ProtoPirate/<nazwa>.log
```

- `.sub` — oryginalny zdekodowany zapis
- `.state` — aktualny licznik, krok i kod przycisku
- `.log` — historia nadanych liczników

Po restarcie urządzenia kolejne nadanie kontynuuje od licznika zapisanego w pliku `.state`.

## Zakres KeeLoq

Regeneracja rolling code KeeLoq jest obsługiwana tylko wtedy, gdy:

- zdekodowany protokół to `KeeLoq`
- producent został rozpoznany
- dostępny jest numer seryjny

Nieznani producenci KeeLoq pozostają w trybie replay, ponieważ niezawodna regeneracja rolling code nie może być zagwarantowana.

Pozostałe protokoły RF nadal korzystają z istniejących ścieżek dekodowania, kodowania i emulacji.

## Przetestowany przykład

Workflow KeeLoq Remote został przetestowany z pilotem DTM_Neo.

Trzy ramki wygenerowane przez ProtoPirate pokryły się z trzema kolejnymi ramkami fizycznego pilota:

- wartości licznika były zgodne
- wygenerowane wartości `Key` były zgodne
- wygenerowane wartości `Hop` były zgodne
- `Serial`, kod przycisku i część stała pozostały zgodne

## Projekt nadrzędny

Ten fork jest oparty na Bruce:

https://github.com/pr3y/Bruce

Ogólna dokumentacja Bruce, obsługiwany sprzęt, instrukcje instalacji i pełna lista funkcji znajdują się w projekcie nadrzędnym:

https://github.com/pr3y/Bruce  
https://wiki.bruce.computer/

## Zastrzeżenie

Ten fork jest przeznaczony wyłącznie do legalnych i autoryzowanych badań bezpieczeństwa, edukacji oraz testów. Nie należy używać go do zakłócania działania systemów, których nie jesteś właścicielem albo do których nie masz wyraźnego uprawnienia.

Nadrzędny projekt Bruce jest dystrybuowany na licencji Affero General Public License (AGPL). Ten fork przestrzega tych samych wymagań licencyjnych.
