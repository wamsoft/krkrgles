# gles — OpenGL ES 描画アダプタープラグイン (吉里吉里Z)

吉里吉里Z (krkrz) 用のプラグイン (`gles.dll`)。OpenGL ES (ANGLE/EGL 経由) で描画した結果を吉里吉里のレイヤへ吸い上げ、吉里吉里側でさらに別画像と合成できるようにする。Live2D / M2Motion など GLES 実装をもつ素材の、単体・複数の合成結果を得る用途を想定している。

Windows では ANGLE を介するため、実際のバックエンドは Direct3D (既定 D3D11、`forceD3D9` 指定で D3D9)。

## 特長

- レイヤ内容を GLES テクスチャ化し、アフィン変換・ブレンドモード付きで FBO へ描画
- 描画結果をレイヤのビットマップへ読み戻し (吉里吉里側で通常合成に利用可)
- **ポストエフェクト機構** — 描画を中間バッファに捕捉し、画像加工コマンド列を GPU 上で適用してから合成。加工は CPU 読み戻しを介さず GPU 内で完結する

## ビルド

CMake プリセット + vcpkg。`Makefile` がラッパになっている。

```sh
make            # 設定 + ビルド (OS からプリセット自動選択)
make prebuild   # cmake 構成のみ
make build      # ビルドのみ
make clean
```

- `VCPKG_ROOT` 環境変数が必要。
- プリセットは OS から自動選択 (Windows は `x64-windows`)。明示する場合: `make build PRESET=x86-windows BUILD_TYPE=Debug`。
- 出力は `build/<preset>/`。

### 依存 (本リポジトリにはベンダリングされていない兄弟ディレクトリ)

`CMakeLists.txt` が `../tp_stub/krkrz.cmake` を include し、そこから `krkrz_plugin()` マクロと `NCBIND` を得る。よって以下が本リポジトリの兄弟として必要:

- `../tp_stub/` — 吉里吉里Z プラグインスタブ (`tp_stub.h`, `krkrz.cmake`)。`-DTPSTUB_DIR=...` で位置変更可。
- `../ncbind/` — ncbind バインディングフレームワーク (`NCBIND` フラグで取り込まれる)。

`glad/` (EGL + GLES2 ローダ) は本リポジトリに同梱、静的ライブラリとしてビルドされる。

> ソースは UTF-8。コメントは日本語。MSVC では本プロジェクトが最上位のとき `/utf-8` が付く (`CMakeLists.txt`)。吉里吉里ツリーの一部としてビルドする場合は親側でエンコーディング指定が必要 (無いと日本語コメントが C4828 警告)。コード中の文字列リテラルは全て ASCII。

## クラス

ncbind で吉里吉里Z へ公開されるクラスは 2 つ。詳細な引数は `manual.tjs` を参照。

### GLESAdaptor

メインオブジェクト。EGL ディスプレイ/コンテキスト/サーフェスと、結果格納用のオフスクリーン FBO を保持する。

```tjs
var adaptor = new GLESAdaptor(window [, forceD3D9]);
```

| メンバ | 説明 |
|---|---|
| `setScreenSize(w, h)` / `screenWidth` / `screenHeight` | 描画解像度 |
| `makeCurrent()` | GL コンテキストを current に |
| `capture(layer, callback, param, color)` | `color` でクリアした FBO 上で `callback(w, h, param)` を実行し、結果を `layer` へ読み戻す |
| `drawLayer(layer, a, b, c, d, tx, ty [, opacity])` | レイヤ/GLESTexture をアフィン変換 (2x2 = a,b,c,d、平行移動 tx,ty) で描画 |
| `copyLayer(layer, left, top)` | 等倍コピー (`drawLayer` の簡易版) |
| `blendMode` | ブレンドモード (`tTVPBlendMode` 相当) |
| `beginEffect()` / `endEffect(commands)` | ポストエフェクト (下記) |

### GLESTexture

レイヤ内容から事前生成するテクスチャ。同じ素材を繰り返し描画する際に再アップロードを省ける。

```tjs
var tex = new GLESTexture();
tex.load(layer);        // レイヤ内容を取り込む
// adaptor.drawLayer(tex, ...) で描画
```

## ポストエフェクト

`beginEffect()` 〜 `endEffect(commands)` で囲んだ描画を中間フレームバッファ (透明クリア済み) に捕捉し、`commands` (加工コマンドの配列) を順に適用してから、現在の `blendMode` で直前の描画先へ合成する。`capture()` のコールバック内で、モジュール描画ごとに個別のエフェクトを掛けられる (ネスト可)。Live2D / M2Motion などの自前 GL 描画も捕捉対象。

```tjs
adaptor.capture(dest, function(w, h, p) {
    drawLayer(bgLayer, 1,0,0,1, 0,0);          // 背景は加工なし

    beginEffect();
        drawLayer(charA, 1,0,0,1, 100,50);     // 捕捉対象
    endEffect([
        %[ cmd:"grayscale" ],
        %[ cmd:"gamma", value:1.3 ],
    ]);
}, void, 0x00000000);
```

### 処理の分類 (内部構造)

| 種別 | コマンド | パス構成 |
|---|---|---|
| LUT 系 | `gamma` / `adjustGamma`, `light`, `lut` | per-channel 256 段 LUT に帰着。連続するものは CPU 側で 1 枚に合成し 1 パス |
| 混合系 | `grayscale`, `colorize`, `modulate`, `noise`, `generateWhiteNoise`, `overcolor` | オペコードループ 1 パスに融合 |
| 近傍系 | `boxBlur`, `gaussianBlur` | 重み付き分離畳み込み (H/V 2 パス、中間 FBO で ping-pong) |

チェーンコンパイラはカテゴリ境界で融合を区切る。読み戻しは `capture` 末尾の 1 回だけ。

### コマンド一覧

```
点処理 (連続すると 1 パスに融合):
  %[ cmd:"grayscale" ]
      グレースケール化  Y = (19*B + 183*G + 54*R) >> 8

  %[ cmd:"gamma", value:γ ]                         // 全チャンネル一括
  %[ cmd:"gamma", rgamma:.., rfloor:.., rceil:..,
                  ggamma:.., gfloor:.., gceil:..,
                  bgamma:.., bfloor:.., bceil:.. ]   // チャンネル別
      ガンマ補正  out = pow(in/255, 1/γ)*(ceil-floor)+floor
      (本体 Layer.adjustGamma 準拠。floor 既定 0 / ceil 既定 255)

  %[ cmd:"light", brightness:b, contrast:c ]
      明度 b(-255〜255) / コントラスト c(-100〜100)

  %[ cmd:"lut", table:[ ...256要素... ] ]
      全チャンネル共通の 256 段ルックアップテーブル

  %[ cmd:"colorize", hue:h, sat:s, blend:b ]
      色相 h(0〜255) / 彩度 s(0〜255) / 合成率 b(0〜1)

  %[ cmd:"modulate", hue:h, saturation:s, luminance:l ]
      色相 h(-180〜180) / 彩度 s(-100〜100) / 輝度 l(-100〜100)

  %[ cmd:"noise", level:n ]
      ノイズ付加 n(0〜255)

  %[ cmd:"generateWhiteNoise" ]
      グレースケールのホワイトノイズ生成 (α は保持)

  %[ cmd:"overcolor", color:0xAARRGGBB, type:mode, opacity:o ]
      指定色での全体塗りつぶし合成 (吉里吉里 fillOperateRect 準拠)。
      合成αは color 上位8bit(AA) × opacity(0〜255, 既定255)。
      type は吉里吉里の合成モード値 (tTVPBlendOperationMode):
        1=Opaque 2=Alpha(既定) 3=Additive 4=Subtractive 5=Multiplicative
        8=Dodge 9=Darken 10=Lighten 11=Screen 12=AddAlpha
        13=PsNormal 14=PsAdditive 15=PsSubtractive 16=PsMultiplicative
        17=PsScreen 18=PsOverlay 19=PsHardLight 20=PsSoftLight
        21=PsColorDodge 22=PsColorDodge5 23=PsColorBurn
        24=PsLighten 25=PsDarken 26=PsDifference 27=PsDifference5
        28=PsExclusion

近傍処理 (中間バッファを介して H/V 2 パス):
  %[ cmd:"boxBlur", area:r, iter:n ]
      半径 r の一様ぼかしを n 回 (iter 既定 1)

  %[ cmd:"gaussianBlur", radius:r ]
      半径 r のガウスぼかし
```

各演算は吉里吉里本体 (`TVPDoGrayScale` / `TVPAdjustGamma`) と LayerExImage プラグイン、および吉里吉里 GPU 描画の blendmode GLSL に合わせて実装している。ただし以下は原理的に完全一致せず、視覚的同等を狙う:

- `noise` / `generateWhiteNoise` — CPU の `rand()` 列は再現不能なため座標ハッシュで代替
- `boxBlur` / `gaussianBlur` の最外周 — CPU は端で重み再正規化、GPU は `CLAMP_TO_EDGE`
- `colorize` — CPU は整数 HSL 丸め、GPU は同アルゴリズムの浮動小数版

## 関連ファイル

- `manual.tjs` — API リファレンスと使用サンプル
- `src/GLEffect.{h,cpp}` — ポストエフェクト機構
- `src/GLES.cpp` — `GLESAdaptor` / `GLESTexture` 本体
- `CLAUDE.md` — 開発者/エージェント向けの内部メモ
