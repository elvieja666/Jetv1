# Jet

Flanger "jet plane" con modo **through-zero** (como el flanging de cinta).
Una copia de la señal pasa por un delay muy corto que barre lentamente; al sumarla
con la original se forma un filtro peine que se mueve. Con feedback alto suena
metálico ("jet"). En modo through-zero la copia cruza a la original hacia adelante
y hacia atrás: las cancelaciones convergen a cero y se dan vuelta ("whoosh").

## Controles

| Control      | Qué hace |
|--------------|----------|
| Rate         | Velocidad del barrido (0.01–5 Hz, por defecto 0.12 Hz: lento, tipo jet). |
| Depth        | Cuánto recorre el barrido (por defecto 90%). |
| Manual       | Delay central / máximo recorrido (0.2–10 ms, por defecto 4 ms). Más alto = barrido más grave. |
| Feedback     | Realimentación (-95% a +95%). Positivo = jet metálico; negativo = más hueco. |
| Tone         | Pasabajos dentro del feedback: más bajo = más oscuro, tipo cinta. |
| Spread       | Desfase del barrido entre L y R (0° = mono, 180° = opuestos). |
| Through-Zero | Modo cinta: la copia cruza a la original. Agrega 10.5 ms de latencia (se informa al DAW, que la compensa). |
| Mix          | Seco / flangeado. 50% = cancelaciones más profundas. |
| Output       | Nivel de salida. |

## Compilar en Mac

```bash
xcode-select --install        # una sola vez
brew install cmake            # una sola vez (o bajar CMake desde cmake.org)
cd ~/Music/Jet
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j8
killall -9 AudioComponentRegistrar 2>/dev/null; echo listo
```

## Compilar en GitHub (sin Xcode)

Subí el proyecto a un repo (incluida la carpeta `.github/workflows/build.yml`).
En **Actions** → último run → **Artifacts** → `Jet-macOS` se descargan `Jet-AU.zip` y `Jet-VST3.zip`.

```bash
cd ~/Downloads
unzip -o Jet-AU.zip -d ~/Library/Audio/Plug-Ins/Components/
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/Components/Jet.component
killall -9 AudioComponentRegistrar 2>/dev/null; echo listo
```
