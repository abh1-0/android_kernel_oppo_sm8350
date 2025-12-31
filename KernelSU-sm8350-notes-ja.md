# KernelSU 統合メモ (SM8350 / kernel 5.4, GKI 1.0)

このリポジトリに KernelSU v4.1.0 を統合した際の修正点とビルド手順を日本語でまとめます。対象端末は OnePlus 9 Pro (lahaina, qgki variant) で、GKI 5.4 カーネルです。

## 取得バージョン
- `KernelSU` ディレクトリ: タグ `v4.1.0`
- 親カーネル: 5.4 GKI 1.0 (lahaina qgki)

## カーネル側の互換パッチ概要
- `kernel/kernel_compat.h` を追加・拡張  
  - task_work 用の `TWA_*` マクロ定義  
  - `copy_from_user_nofault` / `strncpy_from_user_nofault` 互換実装  
  - `uaccess.h` / `sched/task.h` など 5.4 で欠けるヘッダを吸収  
- 互換ヘッダを各ファイルで包含 (`allowlist`, `dynamic_manager`, `kernel_umount`, `sulog`, `throne_comm`, `ksud`, `supercalls`, `syscall_hook_manager`, `util`, `sucompat` ほか)
- `allowlist` / `dynamic_manager`: `put_task_struct` 利用と `sched.h` 追加
- `app_profile`: `current->seccomp.filter_count` アクセスを 5.9 未満でガード
- `pkg_observer`: 5.4 の `fsnotify_ops.handle_event` シグネチャに合わせて `handle_inode_event` を使用しない形へ修正
- `supercalls`: 5.4 には `anon_inode_getfd_secure` が無いため `anon_inode_getfd` にフォールバックし、引数個数も 4 引数に修正
- `sucompat` / `util`: `asm/pgtable.h` と互換ヘッダを使用して `linux/pgtable.h` 欠如を回避
- SELinux 関連  
  - `sepolicy.c`: `add_filename_trans` はカーネル >=5.10 のみ有効。それ未満は `false` を返してビルドを通す。  
  - `rules.c`: 5.4 のレイアウトに合わせ `selinux_state.ss` から `policydb` を取得し、NULL 時は警告ログのみ。

## ビルド手順
```
BUILD_CONFIG=build.config.lemonade VARIANT=qgki LTO=full BUILD_KERNEL=1 build/build.sh
```
- 成果物: `out/msm-5.4-lahaina-qgki/dist/` に `Image`, `dtbo.img`, `lahaina*.dtb`

## AnyKernel3 でのパッケージング
- 使った AK3: `/home/tqmane/git/AnyKernel3`
- 作成した ZIP: `/home/tqmane/git/kernel-ksu-5.4.zip`
  - `Image`・`dtbo.img` を同梱
  - `lahaina.dtb` を `dtb` として同梱

## 既知の事象
- KernelSU 有効時のブートループはカーネルパニックではなく、Zygisk モジュール（`zygisk_lsposed` / `zygisk-su`）が Zygote を SIGSEGV させていた。モジュールの無効化・更新で解消。pstore は空で、カーネル側クラッシュは未確認。

## なぜ「そのまま」では 5.4 でビルドできなかったか
5.4 より新しいカーネル API/構造体を前提にしたコードが複数あり、以下のようなコンパイルエラーが出た。
- `TWA_RESUME` など task_work のシンボルが未定義（5.6 以降）。
- `copy_from_user_nofault` / `strncpy_from_user_nofault` が 5.4 には無い。
- `anon_inode_getfd_secure` が無く、引数個数も異なる。
- `fsnotify_ops` に `handle_inode_event` フィールドが無い（5.4 は `handle_event` シグネチャ）。
- seccomp の `current->seccomp.filter_count` が 5.9 未満に存在しない。
- SELinux の `filename_trans_*` 構造・API が 5.10 以降向けで、5.4 では型未定義。
- `linux/pgtable.h` が 5.4 ではパス違いで見つからず（`asm/pgtable.h` が正）。
- `put_task_struct` やヘッダ不足で警告→エラーになっていた。

## その修正内容（詳細）
- `kernel/kernel_compat.h` を作り、上記の欠落 API/TWA マクロを提供。`copy_from_user_nofault`/`strncpy_from_user_nofault` 互換、必要ヘッダの集中 include。
- 各ファイルで compat ヘッダを include（allowlist, dynamic_manager, kernel_umount, sulog, throne_comm, ksud, supercalls, syscall_hook_manager, util, sucompat など）。
- fsnotify: `pkg_observer.c` を 5.4 の `handle_event` シグネチャに合わせて書き換え。
- seccomp: `app_profile.c` で `filter_count` は 5.9 未満では触らないようガード。
- anon_inode: `supercalls.c` で `anon_inode_getfd_secure` 不在時に `anon_inode_getfd` を使い、4 引数に合わせた。
- task_work/put_task_struct: allowlist/dynamic_manager で `sched.h` を追加し、`put_task_struct` を正しく呼ぶ。
- pgtable: `sucompat.c` と `util.c` は `asm/pgtable.h` を include し、compat ヘッダ経由で不足を補う。
- SELinux:  
  - `sepolicy.c` は `add_filename_trans` を 5.10 以上に限定し、それ未満ではダミー返却。  
  - `rules.c` は 5.4 の `selinux_state.ss` から policydb を取得し、NULL 時は警告ログのみ。

## もし upstream/push する場合のメモ
- `KernelSU` ディレクトリ内で: `git push origin main`（互換パッチを公開する場合）
- 親カーネル側で: サブモジュールポインタ更新後に `git push`（ブランチ例: `oneplus/sm8350v_15.0.0_oneplus9pro_sukisu`）

## デバッグのヒント
- KernelSU 無効では起動する場合、まず Zygisk/LSPosed/モジュールを疑う。`/data/adb/modules/<name>/disable` を置くなどで切り分け。  
- カーネルクラッシュを疑うときは pstore（`/sys/fs/pstore/`）と kmsg を確認。今回のケースでは pstore は空だった。
