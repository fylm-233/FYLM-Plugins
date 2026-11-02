[English](README.md) · [简体中文](README.zh-CN.md) · [日本語](README.ja.md)

# FYLM Plugins

**Windows 版 Adobe After Effects** 用のピクセルアート、レトロディスプレイ、アナログ映像エフェクト集です。

3つの独立したエフェクトを **FYLM Plugins** カテゴリにまとめています。単独でも、AE のエフェクトスタックで組み合わせても使えます。

| エフェクト | 主な機能 |
|---|---|
| **FYLM Pixel Art** | ディザリング、印刷網点、PC-98、GBA、現代的なピクセルアート。ピクセルサイズ、減色、細部の整理を調整でき、12 スタイルと 24 種類のパレットを収録。 |
| **FYLM CRT VHS** | CRT、VHS、両者の組み合わせ。走査線、画面の湾曲、色にじみ、ノイズ、動くテープ干渉を調整でき、8 種類の VHS スタイルを収録。 |
| **FYLM ShaderGlass** | ディスプレイ、携帯ゲーム機、アナログ信号、ピクセル拡大、色調の計 38 プリセット。カテゴリ、検索、お気に入りに対応し、入力解像度、強度、時間、プリセットのパラメーターを調整。 |

| Pixel Art · PA02 A | ShaderGlass · SG02 | CRT VHS · VH05 |
|---|---|---|
| ![PA02 A](assets/presets/pixel-art/thumbnails/PA02-A.png) | ![SG02](assets/ShaderGlass/Library/thumbnails/sg02.png) | ![VH05](assets/presets/vhs/thumbnails/VH05.png) |

プレビューには FYLM オリジナルの診断画像を使用しています。AE の操作画面ではありません。

## 使い方

1. AE と aerender を終了し、Windows テストパッケージを展開して、管理者 PowerShell で同梱の `install.ps1` を実行します。
2. AE を再起動し、「エフェクト → FYLM Plugins」から追加します。
3. モードやプリセットを選び、調整します。表示言語は AE の設定に自動で従います。

標準の「エフェクトコントロール」はプレビューに直接反映されます。任意の「ウィンドウ → FYLM Plugins」パネルでは「追加／接続 → 調整 → パネル設定を適用」の順で操作します。このパネルに VHS スタイルを読み込んだ場合も、最後に適用してください。

ShaderGlass は標準コントロールから対応する `.slangp` ソースを読み込み、コンパイル済みの `.fylmsg` を読み込み・書き出しできます。対応状況はプリセットによって異なり、過去フレームを必要とするシェーダーには対応していません。ピクセル拡大の違いを見るには、入力グリッドを粗く設定してください。

## 現在の状態

Windows 向け開発候補版です。0.13 の新機能と今回の UI 変更は、AE 上での確認が必要です。macOS とすべての `.slangp` への対応は含まれません。[公開前の確認](docs/PUBLICATION-REVIEW.md)と[ビルド手順](docs/BUILDING.md)を参照してください。

## 参照プロジェクト

- [Dithering Studio](https://github.com/Oslonline/dithering-studio)
- [godot-shader-crt-vhs](https://github.com/punikonta/godot-shader-crt-vhs)
- [ShaderGlass / ShaderGC](https://github.com/mausimus/ShaderGlass)
- [libretro/slang-shaders](https://github.com/libretro/slang-shaders)
- [ntsc-rs](https://github.com/valadaptive/ntsc-rs)
- reimuplayer halftone — [provenance / 来源 / 出典](references/reimu-halftone)

ntsc-rs は信号処理の概念を参照しています。Rust ランタイムは含まれません。

本リポジトリには異なるライセンスのコンポーネントが含まれます。[NOTICE](NOTICE)、[Apache-2.0](LICENSE)、[GPL-3.0](LICENSE.GPL-3.0.txt)、各ソースの表記を確認してください。ルートのライセンスは全コンポーネントを対象とするものではありません。公開前の出典確認は[公開前の確認](docs/PUBLICATION-REVIEW.md)で管理しています。

