# [Project Mulus](https://github.com/kahnn/Mulus)

## 1. プロジェクト概要

Project Mulus (以降 Mulus) は、[エコーネットコンソーシアム](http://www.echonet.gr.jp/)による[ECHONET Lite 規格 Ver1.01](http://www.echonet.gr.jp/spec/spec_v101_lite.htm)をサポートする機器実現のための、C言語で書かれた ToolKit です。
元々は個人的な家電Hackのために始めましたが、今後普及が進むと考えられる HEMS において、自家製家電を組み入れたり、自分の好みで市販家電の制御を行ったりしたい愛好家の手助けになることを目指しています。


## 2. 機能一覧＆サポート状況

ECHONET Lite規格に対する Mulusのサポート状況については、以下の通りです。
まだ、Mulus は開始されたばかりです。(2013/01/29現在 α版未リリース)

```
---------------------------------------------------------
- ECHONET Lite サポート仕様制限

SetGetについてはエラー（返却なし）とする
インスタンスコードが指定の際の処理が無い。
EPR処理で個数制限がある場合の処理が無い。
他ノードの情報(proxy)を保持することはできない。必要があれば、自分で処理する。

---------------------------------------------------------
- Mulus実装制限

アーキテクチャとしてMultiThreadには対応していない。MTSafeでもないので気をつけること。
シングルスレッドモデルなので、タイマー処理の中や、各プロパティ処理の中で長時間処理をおこなったりsleepしたりしないこと。
タイマー処理間隔は10秒固定だが、他の処理などにも影響され、必ずしも正確な時間にはならない。したがって、必要に応じてハンドラ中で時間の確認を行う事。

---------------------------------------------------------
- サンプルの仕様
  温度センサー サンプル および コマンド仕様については、
  各ディレクトリのREADMEファイルを参照してください。
```


### **通信仕様について**

```
Layer4以下の通信プロトコル (第1章 概要)
  UDP/IP マルチキャスト(V4のみ)をサポートし、
  IPアドレスは 224.0.23.0 , ポート番号は 3610 とする。
  - MWARE P.1-2
  
他ノードのECHONETオブジェクトの扱い (第2章 ECHONETオブジェクト)
  他ノード上に存在するECHONETオブジェクトのコピーを通信ミドルウェア上で
  保持することはしない。状態取得、制御、通知はすべて同期的な動作のみを提供
  するため、非同期動作を実現する場合は上位のアプリケーションでおこなう。
  - MWARE P.2-3 - 2-8

フレームフォーマット (第3章 電文構成)

 ECHNET Lite ヘッダ
  EHD1  (1b)  :  0x10 = ECHONET Lite 規格
  EHD2  (1b)  :  0x81 = 規定電文形式
  - MWARE P.3-2

 Transaction ID
  TID   (2b)  :  0からインクリメント
  - MWARE P.3-3

 EDATA
  EOJは、クラスグループ、クラス、インスタンス(0x00は全インスタンス)で3byte。
  全インスタンス指定による要求に対する応答は、インスタンス単位でおこなう。(MWARE P.3-6)
  SEOJ,DEOJとして設定するEOJが無い場合はノードプロファイルクラスを指定する。(MWARE P.3-6)

  SEOJ   (3b) : Source EOJ
  DEOJ   (3b) : Destination EOJ
  - MWARE P.3-3 - 3-5

  ESV    (1b) : サービスコード
    ESVコード一覧は、MWARE P.3-6,7 参照。

    記号       ESV     内容                       備考
    -----------------------------------------------------
    SetI       0x60    書き込み要求(応答不要)  一斉同報可
    SetC       0x61    書き込み要求(応答要)    一斉同報可
    Get        0x62    読み出し                一斉同報可
    INF_REQ    0x63    通知要求                一斉同報可
    SetGet     0x6E    書き込み・読み出し要求  一斉同報可
    -----------------------------------------------------
    Set_Res    0x71    書き込み応答            0x61の応答
    Get_Res    0x72    読み出し応答            0x62の応答
    INF        0x73    通知(応答不要)          0x63の応答または自発、一斉同報可
    INFC       0x74    通知(応答要)            自発
    INFC_Res   0x7A    通知応答                0x74の応答
    SetGet_Res 0x7E    書き込み・読み出し応答  0x6Eの応答
    -----------------------------------------------------
    SetI_SNA   0x50    書き込み要求不可応答    0x60の応答
    SetC_SNA   0x51    書き込み要求不可応答    0x61の応答
    Get_SNA    0x52    読み出し要求不可応答    0x62の応答
    INF_SNA    0x53    通知要求不可応答        0x63の応答
    SetGet_SNA 0x5E    書込・読出要求不可応答  0x6Eの応答
    -----------------------------------------------------

    処理は以下の原則に従う。 (MWARE P.3-9 - 3-15)

    (1) DEOJが存在しない
      応答は返さない

    (2) 要求を受け付けない -or- DEOJは存在するが指定されたEPCが存在しない
      要求(0x6*)に対応する不可応答(0x5*)を返す。EPCについては、(3)参照。
      その際に応答先アドレスは要求元の(下位通信レイヤでの)アドレスを使用する。
      なお、SetGet(0x6E)をサポートしない場合は、OPCSet=0、OPCGet=0を設定して、不可応答(0x5E)を返す。

    (3) EPCの処理
      書き込み要求では、PDCのEDTのデータ長が設定される。
      読み込み要求では、PDCには0が設定されEDTは付かない。
      処理側は、EPCの数が多いか見つからなかった場合のエラー通知として、要求されたそのEPCのPDC,EDTをそのまま返す。
      処理が正しく行えたものは、書き込みの場合はPDCに0を設定し、EDTを"外して"返す。
      読み出しの場合は、EDTを設定する。
      
    (4) 通知
      自発または通知要求(0x63)に対する成功通知(0x73)は、一斉同報になる。
      自発的通知の場合は、DEOJに設定すべきEOJが無いため、ノードプロファイルクラスを格納する。


  OPC    (1b) : SetGet_SNAのみ0をとる。それ以外は1以上。
  - MWARE P.3-16

  EPC n  (1b) : ECHONET Property Code
  PDC n  (1b) : Property Data Counter (EDTのバイト数)、読み出しの際は 0指定。
  EDT n  (*b) : ECHONET Property Data


ECHONET Lite ノード立ち上げ処理  (第4章 基本シーケンス)
  ノード立ち上げ時には、インスタンスリストを一斉同報で通知する。
  ノードのインスタンス数がMAX値を超える場合は複数の電文で通知する。
  電文の内容は以下の通り。
  * SEOJ, DEOJはノードプロファイルオブジェクト (0x0EF001) を指定
  * ESV で通知 (0x73) を指定。
  * EPCでインスタンスリスト通知プロパティ (0xD5) を指定
  * EDTに自ノード内のインスタンスリスト情報を指定
  - MWARE P.4-7


処理機能とサポート範囲は以下の通り。 (MWARE P.5-3 の表を参照)

  M2 a  基本処理                    ○
     b  Set処理                     ○
     c  Get処理                     ○
     d  通知処理                    ○
     e  SetGet処理                  ×

     A  Get拡張処理                 ×
     B  SetGet拡張処理		                ×
     C  通知拡張処理                ×
     E  他機器オブジェクト管理(1)   ×
     F  他機器オブジェクト管理(2)   ×
     G  他機器オブジェクト管理(3)   ×
     H  他機器オブジェクト管理(4)   ○
     I  自機器オブジェクト管理(1)   ×
     J  自機器オブジェクト管理(2)   ○

  M5 a  電文作成・管理処理          ○


プロファイルオブジェクト (第6章 ECHONET オブジェクト詳細規定)

  プロファイルオブジェクトスーパークラス (グループ=0x0E)
    プロファイル共通のプロパティ規定では、以下が必須。
    * メーカーコード(0x8A)  (3b)  Get 
    * 状態アナウンスプロパティマップ(0x9D)  (MAX17b)  Get
    * Setプロパティマップ(0x9E)  (MAX17b)  Get
    * Getプロパティマップ(0x9F)  (MAX17b)  Get
    -MWARE P.6-4,5

  ノードプロファイルクラス (グループ=0x0E、クラス=0xF0、インスタンス=0x01)
    ノード内にインスタンスが１つあるので、インスタンスコードは 1。
    基本的にはすべて必須プロパティ。
    -MWARE P.6-6 - 6-10


```
[MWARE：第2部 ECHONET Lite 通信ミドルウェア仕様](http://www.echonet.gr.jp/spec/pdf_v101_lite/ECHONET-Lite_Ver.1.01_02_.pdf)

### **サポートオブジェクトについて**

```
機器オブジェクトスーパークラス (OBJD P.2-1)
温度センサクラス  (OBJD P.3-27)
```
[OBJD：APPENDIX ECHONET機器オブジェクト詳細規定 Release B](http://www.echonet.gr.jp/spec/spec_app_b.htm)内で定められている


## 3. 利用方法

Mulus の利用者は、動作対象の機器に応じて、Mulus のコード全てをそのまま使うのではなく、目的に応じて必要なコードを選択し使用することができます。むしろ、Mulus を部品群として、必要に応じてパーツ取りして役立ててもらえればと思います。
開発・動作確認は、Linux(2.6.32) 上でおこなっていますが、Linux固有のシステムコール等は基本的に使用しないようにしていますので、UNIX系のOS上であれば修正は軽微だと思います。
* 温度センサーなどの機器を作る場合は `*/*/*.c` を参考にしてください。
* 各機器を制御するコントローラを作る場合は `*/*/*.c` を参考にしてください。

参考までに、主なソース構成を以下に示します。

```
  TBD
  - include
  - tool
  - middleware
  - util
```


## 4. 機器オブジェクトの追加

Mulus で用意されていない機器オブジェクトを追加する場合は、以下のようにおこないます。

```
  TBD
  - 該当機器オブジェクト用のソース[\*.hc]を作成
  - 固有プロパティの追加
  - ノードへの追加
  - プロファイルへの追加
  - イベントハンドリング
```


## 5. FAQ

* CentOS 6 でマルチキャストアドレスのパケットが受信できない
  127.0.0.1 にバインドすれば上手くいく場合は、Firewall ではじかれている可能性が高い。
  iptablesの設定を変更して、マルチキャストパケットが通るようにする。
  以下にデフォルトから修正した場合の概要を示す。(実際には、もっと対象を絞った方が良い)

```
# /etc/sysconfig/iptables の内容
------------------------------
# Firewall configuration written by system-config-firewall
# Manual customization of this file is not recommended.
*filter
:INPUT ACCEPT [0:0]
:FORWARD ACCEPT [0:0]
:OUTPUT ACCEPT [0:0]
-A INPUT -m state --state ESTABLISHED,RELATED -j ACCEPT
-A INPUT -p icmp -j ACCEPT
-A INPUT -i lo -j ACCEPT
-A INPUT -m state --state NEW -m tcp -p tcp --dport 22 -j ACCEPT
##### ADD START HERE
-A INPUT -m pkttype --pkt-type multicast -j ACCEPT
-A INPUT --protocol igmp -j ACCEPT
## needed for unicast 
-A INPUT -p udp -j ACCEPT
-A OUTPUT -p udp -j ACCEPT
## needed for multicast ping responses
#-A INPUT -p icmp --icmp-type 0 -j ACCEPT
##### ADD END HERE
-A INPUT -j REJECT --reject-with icmp-host-prohibited
-A FORWARD -j REJECT --reject-with icmp-host-prohibited
COMMIT
------------------------------

# 設定の反映

# /etc/init.d/iptables restart
# iptables -L
```


## 6. Author

**Kahnn Ikeda**
* https://github.com/kahnn


## 7. Copyright and license

Mulus is free software, available under the terms of a [MIT license](http://opensource.org/licenses/mit-license.php).
Please see the LICENSE file for details.

## 8. リリース

2013/02/?? α版リリース
