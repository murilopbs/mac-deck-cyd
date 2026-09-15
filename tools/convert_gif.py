#!/usr/bin/env python3
"""
MacDeck CYD - GIF Optimizer & Header Generator
Redimensiona, reduz taxa de quadros e converte um GIF para array C em PROGMEM.
"""

import sys
import os
import io
from PIL import Image, ImageSequence

INPUT_GIF = "brina.gif"
OUTPUT_HEADER = "ScreensaverCustom.h"

def convert():
    if not os.path.exists(INPUT_GIF):
        print(f"❌ Arquivo '{INPUT_GIF}' não encontrado na pasta atual.")
        sys.exit(1)

    print(f"🔍 Carregando '{INPUT_GIF}' ({os.path.getsize(INPUT_GIF) / 1024 / 1024:.2f} MB)...")
    im = Image.open(INPUT_GIF)

    orig_width, orig_height = im.size
    total_frames = getattr(im, "n_frames", 1)
    print(f"📐 Resolução original: {orig_width}x{orig_height}, Total de quadros: {total_frames}")

    # Calcula proporção para largura máxima 240 ou altura 180 (cabendo perfeito no CYD 320x240)
    target_width = 240
    target_height = int((orig_height / orig_width) * target_width)
    if target_height > 200:
        target_height = 200
        target_width = int((orig_width / orig_height) * target_height)

    print(f"✂️ Redimensionando para {target_width}x{target_height}...")

    # Pula frames para manter taxa saudável de ~16-18 fps e economizar Flash
    step = 2 if total_frames > 40 else 1
    frames = []

    for i, frame in enumerate(ImageSequence.Iterator(im)):
        if i % step == 0:
            f = frame.copy().convert("RGBA")
            f.thumbnail((target_width, target_height), Image.Resampling.LANCZOS)
            # Paleta adaptativa de 64 cores (ótima qualidade visual e compressão LZW excelente)
            f_quant = f.convert("P", palette=Image.Palette.ADAPTIVE, colors=64)
            frames.append(f_quant)

    print(f"🎞️ Quadros selecionados: {len(frames)} (Passo: {step})")

    # Salva GIF otimizado em memória
    out_bio = io.BytesIO()
    duration = max(40, frame.info.get("duration", 50) * step)
    frames[0].save(
        out_bio,
        format="GIF",
        save_all=True,
        append_images=frames[1:],
        loop=0,
        duration=duration,
        optimize=True
    )
    gif_bytes = out_bio.getvalue()
    print(f"✨ GIF otimizado: {len(gif_bytes) / 1024:.1f} KB (Redução de {100 - (len(gif_bytes) / os.path.getsize(INPUT_GIF) * 100):.1f}%)")

    # Gera o arquivo C Header
    print(f"📝 Gravando '{OUTPUT_HEADER}'...")
    header_content = [
        "// Auto-gerado por convert_gif.py (NÃO SUBIR PRO GITHUB)",
        "#ifndef SCREENSAVER_CUSTOM_H",
        "#define SCREENSAVER_CUSTOM_H",
        "",
        "#include <Arduino.h>",
        "#include <pgmspace.h>",
        "",
        f"// {target_width}x{target_height} - {len(frames)} frames - {len(gif_bytes)} bytes",
        "const uint8_t screensaver_custom_gif[] PROGMEM = {"
    ]

    bytes_per_line = 16
    for idx in range(0, len(gif_bytes), bytes_per_line):
        chunk = gif_bytes[idx:idx + bytes_per_line]
        hex_line = "  " + ", ".join(f"0x{b:02x}" for b in chunk) + ","
        header_content.append(hex_line)

    header_content.append("};")
    header_content.append("")
    header_content.append("#endif // SCREENSAVER_CUSTOM_H")
    header_content.append("")

    with open(OUTPUT_HEADER, "w", encoding="utf-8") as f:
        f.write("\n".join(header_content))

    print(f"🎉 Sucesso! '{OUTPUT_HEADER}' gerado com {len(gif_bytes)} bytes.")

if __name__ == "__main__":
    convert()
