# マウスオペレーション改

マウスオペレーション改（AdvancedMouseOperation.auf）は、AviUtl 拡張編集のメインウィンドウ上での編集操作を強化する AviUtl プラグインです。

紹介動画は [こちら](https://www.nicovideo.jp/watch/sm0) 。

## 要求環境

- AviUtl version0.99k 以降
- 拡張編集 version0.92
- Microsoft Visual C++ 2015-2022 再頒布可能パッケージ (x86)
    - [こちら](https://docs.microsoft.com/ja-jp/cpp/windows/latest-supported-vc-redist) から最新版を入手できます。本プラグインが読み込まれない場合はアップデートをお試しください。

## 導入方法

[Releaseページ](https://github.com/kumrnm/aviutl-advanced-mouse-operation/releases) の Assets 欄から auf ファイルをダウンロードし、aviutl.exe があるフォルダ（またはその直下の plugins フォルダ）に配置します。

## 機能

メインウィンドウ上での編集操作として、以下のコマンドが追加されます。

|編集内容|操作方法|備考|
|:---|:---|:---|
|回転|Rキー + ドラッグ||
|中心座標移動|Shiftキー + Rキー + ドラッグ||
|水平軸回転|Altキー + Rキー + ドラッグ||
|親グループを選択|Aキー + マウスボタン押下|※1|
|親グループの親グループを選択|Aキー2連打長押し + マウスボタン押下|※1, ※2|
|最前面のオブジェクトを透過|Tキー + マウスボタン押下|※1|
|前面2つのオブジェクトを透過|Tキー2連打長押し + マウスボタン押下|※1, ※2|

|備考||
|:---|:---|
|※1|これらのコマンドは、マウスボタンを押下した瞬間に機能します。すなわち、オブジェクト選択後はキーを離してもドラッグ操作が可能です。|
|※2|3連打以上についても同様に拡張されます。|
|-|上記の操作は併用が可能です。例えば、Aキー + Rキー + ドラッグで、オブジェクトの親グループを回転させることができます。|

また、拡張編集の以下の不具合が修正されます。
- グループ制御が選択されているとき、グループ制御より上のレイヤーに配置されているオブジェクトをメインウィンドウから操作することができない。

## 設定方法

初回起動時、プラグイン本体と同じ階層に `AdvancedMouseOperation.ini` が生成されます。下記の内容に従ってテキストエディタで編集してください。

キーの指定に用いる仮想キーコードについては [こちら](https://docs.microsoft.com/ja-jp/windows/win32/inputdev/virtual-key-codes) をご参照ください。対応するキーが存在しないコード（`0x00` など）を指定すると、当該機能を無効化することができます。

```ini
; 各種キー設定（仮想キーコード）
[key]
rotate=0x52       ; 回転
ancestor=0x41     ; 親グループ選択
transparent=0x54  ; 透過

; バグ修正機能の有効/無効（on または off）
[bugFix]
fixGroupTarget=on

[other]
rotationAmount=0.01            ; 水平軸回転の回転量
repeatedKeyInputTimeLimit=0.3  ; キー連打コマンド入力時の押下間隔の上限（単位：秒）
```

## 注意

- タッチパッド環境では、アルファベットキー押下中のカーソル操作が Windows によって制限されている場合があります。この挙動は、Windowsの設定 > デバイス > タッチパッド からタッチパッドの感度を最大に設定することで解消されます。
- [真・グループ制御](https://github.com/kumrnm/aviutl-authentic-grouping) と併用する場合、真・グループ制御のバージョンが v1.1.0 以上であることを確認してください。**v1.0.0 は本プラグインと競合します**。

## バグ報告

[GitHub Issues](https://github.com/kumrnm/aviutl-advanced-mouse-operation/issues) または [作者のTwitter](https://twitter.com/kumrnm) にお寄せください。

## 開発者向け

- 本リポジトリは Visual Studio 2022 プロジェクトです。
- `test` ディレクトリ直下に `aviutl.exe` などを配置してください。デバッグ環境として使用されます。
