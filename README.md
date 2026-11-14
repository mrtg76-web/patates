# Örgü — polifonik makro osilatör (sürüm 1.0)

Örgü, VST3/AU formatında bir synth plugin'i. Ses motoru, açık kaynak bir Eurorack makro osilatörünün orijinal DSP koduyla çalışıyor. 24 sentez motorunun hepsi donanımla aynı kodla, aynı sesi üretiyor. Örgü bunun üstüne polifoni (8 sese kadar), ADSR, iki LFO, modülasyon matrisi ve preset sistemi ekliyor.

İçindekiler: Kurulum · Ableton'da kullanım · Arayüz · Motorlar · AMP modları · Modülasyon · Presetler · Donanımdan farklar · Derleme · Lisans

## Kurulum (Mac)

1. GitHub'da **Actions** sekmesine gir, en son yeşil **build** çalışmasını aç ve **Orgu-mac-…** paketini indirip aç. İki paket görünürse herhangi biri olur, ikisi de aynı.
2. Dosyaları kopyala:
   - `Orgu.vst3` → `~/Library/Audio/Plug-Ins/VST3/`
   - `Orgu.component` → `~/Library/Audio/Plug-Ins/Components/`
3. Plugin imzasız olduğu için macOS ilk açılışta onu engelleyebilir. Terminal'de şu komutu bir kez çalıştır:
   ```
   xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/Orgu.vst3 ~/Library/Audio/Plug-Ins/Components/Orgu.component
   ```
4. Ableton > Preferences > Plug-Ins bölümünde **Use VST3 Plug-in System Folders** açık olsun, sonra **Rescan**.

Paket "universal": hem Apple Silicon (M1/M2…) hem Intel kodu içeriyor. Bu sayede Rosetta altında çalışan Live 10 da yerel çalışan yeni Live sürümleri de aynı dosyayı kullanabiliyor. `Orgu.app`, DAW olmadan denemek için tek başına çalışan sürüm.

**Windows:** `Orgu-windows` paketindeki `Orgu.vst3` klasörünü `C:\Program Files\Common Files\VST3\` içine kopyala.

## Ableton'da kullanım

1. Bir MIDI track'e **Orgu**'yu instrument olarak yükle. Instruments > Plug-Ins listesinde görünür.
2. Çal. Velocity, pitch bend, mod wheel (CC1), aftertouch ve sustain pedalı (CC64) çalışıyor.
3. Bütün düğmeler otomasyona ve MIDI map'e açık. Ableton'da cihazın üstündeki "Configure" ile istediğin parametreleri cihaz paneline çıkarabilirsin.

## Arayüz

| Bölüm | Ne yapar |
|---|---|
| Sol liste | 24 motor, üç grupta (NEW · PITCHED · NOISE / PERCUSSION). Tıklayınca motor değişir. |
| Üst satır | Motor adı ve o motorda Harmonics / Timbre / Morph'un ne yaptığı. Sağda preset seçici ve **VOICES** (1, 2, 4, 6, 8). |
| Ekran | Canlı dalga formu: turuncu OUT, mavi AUX. Sol üstte o an çalan ses sayısı yazar. |
| FREQUENCY | Tüm notaları ±36 yarım ton kaydırır (0 = çaldığın nota). |
| HARMONICS / TIMBRE / MORPH | Donanımdaki dört ana düğmenin üçü. Her motorda farklı iş yapar, üst satırdaki açıklamaya bak. |
| ENVELOPE → MOD | Donanımdaki üç küçük attenuverter'ın karşılığı. Her notada tetiklenen dahili decay zarfının FM (perde), Timbre ve Morph'a ne kadar gideceğini ayarlar. Ortası 0. |
| LOW PASS GATE | Decay ve Color (rezonans/parlaklık). LPG modunda notanın sönme süresini Decay belirler. |
| AMP | **LPG / ADSR** seçimi ve ADSR düğmeleri (aşağıya bak). |
| PLAY | Glide (portamento) ve pitch bend aralığı. |
| MODULATION | 4 yönlendirme satırı: kaynak → hedef → miktar. |
| LFO · MOD ENV | İki LFO'nun hızı ve dalga şekli, mod zarfının Attack/Decay süresi. |
| OUTPUT | Out/Aux karışımı, ses seviyesi ve **MIX / L-R** seçimi. |

Düğmeler: yukarı/aşağı sürükle, **Alt** ile hassas ayar, **çift tıklama** varsayılan değere döndürür.

## Motorlar

| # | Motor | Harmonics | Timbre | Morph |
|---|---|---|---|---|
| 1 | VA + VCF | filtre rezonansı / karakteri | cutoff | dalga şekli |
| 2 | Phase Distortion | distorsiyon frekansı | miktar | asimetri |
| 3–5 | 6-Op FM I–III | patch (her bankada 32) | modülatör seviyesi | zarf esnetme |
| 6 | Wave Terrain | arazi seçimi | yol yarıçapı | yol kayması |
| 7 | String Machine | akor | chorus / filtre | registrasyon |
| 8 | Chiptune | akor | arpej | pulse width / sweep |
| 9 | Virtual Analog | detune | kare dalga şekli | testere şekli |
| 10 | Waveshaping | shaper dalgası | katlama miktarı | asimetri |
| 11 | 2-Op FM | frekans oranı | modülasyon indeksi | feedback |
| 12 | Formant | formant oranı | formant frekansı | formant genişliği |
| 13 | Harmonic | tepe sayısı | baskın harmonik | tepe şekli |
| 14 | Wavetable | banka | satır | sütun |
| 15 | Chords | akor tipi | çevrim (inversion) | dalga şekli |
| 16 | Speech | sentez tipi | tür (formant kayması) | fonem / kelime |
| 17 | Swarm | perde dağılımı | grain yoğunluğu | grain süresi |
| 18 | Filtered Noise | LP → BP → HP | clock frekansı | rezonans |
| 19 | Particle | frekans dağılımı | yoğunluk | filtre tipi |
| 20 | Inharmonic String | inharmoniklik | parlaklık | sönme |
| 21 | Modal | malzeme | parlaklık | sönme |
| 22 | Bass Drum | vuruş / overdrive | parlaklık | sönme |
| 23 | Snare Drum | ton / gürültü | mod dengesi | sönme |
| 24 | Hi-Hat | metal / gürültü | high-pass | sönme |

Her motorun iki çıkışı var: **OUT** ana ses, **AUX** motorun ikinci varyantı (örneğin Virtual Analog'da sync'li hali, Chords'ta kök nota). OUTPUT'taki Out/Aux düğmesi ikisini karıştırır. **L / R** seçilirse OUT sola, AUX sağa gider; bu bazı motorlarda geniş bir stereo ses verir.

## AMP modları

- **LPG** (varsayılan, donanımdaki gibi): Her nota low pass gate'i "vurur", ses LPG Decay süresinde söner. Nota bırakıldığında ses kesilmez, kendi doğal sönmesini tamamlar. Pluck, perküsyon ve bell sesleri için.
- **ADSR**: Ses, AMP bölümündeki Attack / Decay / Sustain / Release zarfına göre açılıp kapanır. Pad'ler, org, sürekli sesler için. LPG bu modda zarfı takip eder; LPG Decay geçişleri yumuşatır, Color parlaklığı belirler.

Notlar:
- 6-Op FM, string, modal ve davul motorları kendi zarflarını kullanıyor (donanımda da öyle). ADSR modunda Örgü bunlara ayrıca ADSR uyguluyor, böylece Release her motorda çalışıyor.
- Chiptune'da TRIG girişi arpej saati olduğu için bu motor her iki modda da AMP zarfını kullanır.

## Modülasyon

Her satır bir yönlendirme: **kaynak → hedef → miktar** (−100% … +100%).

Kaynaklar: LFO 1, LFO 2, Mod Env (her notada tetiklenen AD zarfı), Velocity, Mod Wheel, Aftertouch, Key Track (C3 merkezli), Random (her notada yeni rastgele değer).

Hedefler: Frequency (%100 = 24 yarım ton), Harmonics, Timbre, Morph, LPG Decay, LPG Color, Level, Engine (%100 = 23 motor ileri).

**Level** hedefinde Velocity, Mod Wheel, Aftertouch ve Mod Env gibi tek yönlü kaynaklar "ne kadar azaltsın" gibi çalışır: kaynak tamamen açıkken seviye değişmez, kapalıyken miktar kadar düşer. Varsayılan ilk satır (Velocity → Level %70) bu yüzden klasik velocity hassasiyeti verir.

Varsayılan yönlendirmeler: Velocity → Level %70, Mod Wheel → Timbre %50, LFO 1 → Frequency %0 (miktarı açınca vibrato olur), Mod Env → Morph %0.

## Presetler

- Üstteki ◀ ▶ fabrika presetleri arasında gezer. Preset adına tıklayınca liste açılır.
- **Save** ayarları `.orgu` dosyası olarak kaydeder. Varsayılan klasör `Belgeler/Orgu Presets`. Bu klasördeki dosyalar listede "User" başlığı altında görünür.
- **Load** başka bir yerden `.orgu` dosyası açar.
- Ableton projesi kaydedilince bütün ayarlar projeyle birlikte saklanır.

21 fabrika preseti var: Init, Glass Wavetable, Analog Pad, Acid Line, Phase Bass, DX Keys, DX Brass, Wave Terrain, String Machine, Chip Arp, FM Bell, Formant Choir, Drawbar Organ, Chord Stabs, Talking, Swarm Drone, Plucked String, Modal Bells, Kick, Snare, Hats.

## Donanımdan farklar

- **CV girişleri yerine modülasyon matrisi var.** V/OCT notadan, TRIG nota başından, LEVEL ise ADSR modunda zarftan geliyor.
- **Polifoni:** Donanım tek ses; Örgü 8 sese kadar çalıyor. VOICES = 1 seçilirse mono/legato çalışır (glide ile).
- **Model düğmesi yerine liste:** 24 motorun hepsi tek tıkla seçiliyor.
- **Örnekleme hızı:** Ses motoru donanımdaki gibi 48 kHz'de çalışıyor. Projen farklı bir hızdaysa (44.1, 96 kHz…) Örgü yüksek kaliteli bir dönüştürücüyle sesi otomatik çeviriyor.
- **Kullanıcı wavetable'ları / 6-op patch yükleme** (donanımda ses dosyasıyla yapılan) yok. Dahili tablolar ve 96 DX7 patch'i mevcut.

## Derleme

```
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
cmake --build build --config Release --target ORGU_VST3 ORGU_AU
```

JUCE 8.0.4 otomatik indirilir. Testler: `cmake -B build -DORGU_BUILD_PLUGIN=OFF && cmake --build build && ctest --test-dir build`.

## Lisans

`third_party/eurorack/` altındaki ses motoru kodu Emilie Gillet'nin açık kaynak Eurorack firmware'inden alındı (MIT lisansı, ayrıntılar orada). Bu proje Mutable Instruments ile bağlantılı değildir ve onun onayını taşımaz.
