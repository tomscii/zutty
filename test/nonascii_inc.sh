# Source: Emacs 'hello' file, visible via M-x view-hello-file (C-h h)

clear; tput smam
tput cup 0 0;
cat <<EOF
Non-ASCII examples:
  Europe: ¡Hola!, Grüß Gott, Hyvää päivää, Tere õhtust, Bonġu
          Cześć!, Dobrý den, Здравствуйте!, Γειά σας, გამარჯობა
  Africa: ሠላም
  East Asia: 你好, 早晨, こんにちは, 안녕하세요
  Misc: Eĥoŝanĝo ĉiuĵaŭde, ⠓⠑⠇⠇⠕, ∀ p ∈ world • hello p  □
  CJK variety: GB(元气,开发), BIG5(元氣,開發), JIS(元気,開発), KSC(元氣,開發)
  Unicode charset: Eĥoŝanĝo ĉiuĵaŭde, Γειά σας, שלום, Здравствуйте!

LANGUAGE (NATIVE NAME)          HELLO
----------------------          -----
Amharic (አማርኛ)                  ሠላም
Armenian (հայերեն)              Բարև ձեզ
Braille                         ⠓⠑⠇⠇⠕
C                               printf ("Hello, world!\n");
Cherokee (ᏣᎳᎩ ᎦᏬᏂᎯᏍᏗ)           ᎣᏏᏲ / ᏏᏲ
Comanche /kəˈmæntʃiː/           Haa marʉawe
Cree (ᓀᐦᐃᔭᐍᐏᐣ)                  ᑕᓂᓯ / ᐙᒋᔮ
Czech (čeština)                 Dobrý den
Danish (dansk)                  Hej / Goddag / Halløj
Dutch (Nederlands)              Hallo / Dag
Efik  /ˈɛfɪk/                   Mɔkɔm

EOF
echo -n "-- Press RETURN --"
read

clear; tput smam
tput cup 0 0;
cat <<EOF
Emacs                           emacs --no-splash -f view-hello-file
Emoji                           👋
English /ˈɪŋɡlɪʃ/               Hello
Esperanto                       Saluton (Eĥoŝanĝo ĉiuĵaŭde)
Estonian (eesti keel)           Tere päevast / Tere õhtust
Finnish (suomi)                 Hei / Hyvää päivää
French (français)               Bonjour / Salut
Georgian (ქართული)              გამარჯობა
German (Deutsch)                Guten Tag / Grüß Gott
Greek (ελληνικά)                Γειά σας
Greek, ancient (ἑλληνική)       Οὖλέ τε καὶ μέγα χαῖρε
Hebrew (עִבְרִית)                  שָׁלוֹם
Hungarian (magyar)              Szép jó napot!
Inuktitut (ᐃᓄᒃᑎᑐᑦ)              ᐊᐃ
Italian (italiano)              Ciao / Buon giorno
Javanese (Jawa)                 System.out.println("Sugeng siang!");
Maltese (il-Malti)              Bonġu / Saħħa
Mathematics                     ∀ p ∈ world • hello p  □
Mongolian (монгол хэл)          Сайн байна уу?
Norwegian (norsk)               Hei / God dag
Polish  (język polski)          Dzień dobry! / Cześć!
Russian (русский)               Здра́вствуйте!

EOF
echo -n "-- Press RETURN --"
read

clear; tput smam
tput cup 0 0;
cat <<EOF
Slovak (slovenčina)             Dobrý deň
Slovenian (slovenščina)         Pozdravljeni!
Spanish (español)               ¡Hola!
Swedish (svenska)               Hej / Goddag / Hallå
Tigrigna (ትግርኛ)                 ሰላማት
Turkish (Türkçe)                Merhaba
Ukrainian (українська)          Вітаю
Vietnamese (tiếng Việt)         Chào bạn

Japanese (日本語)               こんにちは / ｺﾝﾆﾁﾊ
Chinese (中文,普通话,汉语)      你好
Cantonese (粵語,廣東話)         早晨, 你好
Korean (한글)                   안녕하세요 / 안녕하십니까

EOF
echo -n "-- Press RETURN --"
read
