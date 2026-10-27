# Focus Follower

OBS Studio plugin that switches scenes and shows sources based on the focused application.

前面のアプリに応じて、OBS のシーン切替とソースの表示を自動で行うプラグインです。

## Requirements

- Windows
- OBS Studio 31.1.1 or later

## Installation / 導入方法

1. Close OBS Studio.
2. Download `focus-follower-<version>-windows-x64.zip` from [Releases](https://github.com/L4yLa/FocusFollower/releases).
3. Extract the `focus-follower` folder into `C:\ProgramData\obs-studio\plugins\`.
4. Start OBS Studio.
   - Settings: **Tools → Focus Follower**
   - On/off switch: **Docks → Focus Follower**

To uninstall, close OBS Studio and delete the `focus-follower` folder.

---

1. OBS Studio を終了します。
2. [Releases](https://github.com/L4yLa/FocusFollower/releases) から `focus-follower-<バージョン>-windows-x64.zip` をダウンロードします。
3. ZIP 内の `focus-follower` フォルダーを `C:\ProgramData\obs-studio\plugins\` に展開します。
4. OBS Studio を起動します。
   - 設定画面：**ツール → Focus Follower**
   - 有効／無効のスイッチ：**ドック → Focus Follower**

削除するときは、OBS Studio を終了してから `focus-follower` フォルダーを削除します。

## Build

GitHub Actions builds the Windows package on every push to `main`.

## License

GPL-2.0
