# Tips

```shell
# Compile and install into .m2/repository/usbsid/
mvn clean install

# Replace jsidplay2 driver
## MAIN
cp /home/loud/.m2/repository/usbsid/usbsid-usb-driver-library-java/1.0/usbsid-usb-driver-library-java-1.0.jar /mnt/loud/Code/Development/c64/sidplaytrack/jsidplay/svn.jsidplay2-code/jsidplay2/target/standalone/usbsid-usb-driver-library-java-1.0.jar
## THINKPAD
cp /home/loud/.m2/repository/usbsid/usbsid-usb-driver-library-java/1.0/usbsid-usb-driver-library-java-1.0.jar /home/loud/Development/c64/svn.jsidplay2-code/jsidplay2/target/standalone/usbsid-usb-driver-library-java-1.0.jar

# Or in one line
## MAIN
mvn clean install && cp target/usbsid-usb-driver-library-java-1.0.jar ~/Development/c64/sidplaytrack/jsidplay/svn.jsidplay2-code/jsidplay2/target/standalone/
## THINKPAD
mvn clean install && cp target/usbsid-usb-driver-library-java-1.0.jar /home/loud/Development/c64/svn.jsidplay2-code/jsidplay2/target/standalone/usbsid-usb-driver-library-java-1.0.jar
```

# Generate classpath
https://maven.apache.org/guides/introduction/introduction-to-dependency-mechanism.html#Dependency_Scope
https://stackoverflow.com/questions/16655010/in-maven-how-output-the-classpath-being-used
```shell
mvn dependency:build-classpath -Dmdep.includeScope=runtime -Dmdep.outputFile=cp.txt
  -Dmdep.includeScope=compile
  -Dmdep.includeScope=runtime
```

# Jsidplay2 build and test
```shell
# COMPILE
export PATH=$PATH:/opt/apache-maven-3.9.10/bin
mvn clean install

# SHELL FILE
cd target/standalone && cp ../deploy/*.sh . ; chmod +x *.sh && ./jsidplay2.sh -E USBSID && cd ../..
## MAIN
cd target/standalone && cp ../deploy/*.sh . ; chmod +x *.sh && ./jsidplay2-console.sh -E USBSID /mnt/loud/Code/Development/pi/USBSID-Pico-dev/private-repo/code-usbsid-related/configtool_webusb/SID/digitunes/Coma_Light_13_tune_4.sid  && cd ../..
## THINKPAD
cd target/standalone && cp ../deploy/*.sh . ; chmod +x *.sh && ./jsidplay2.sh -E USBSID /home/loud/Development/c64/RETROCOLLECTION/SID-prgs/Commando.prg && cd ../..

# JAVA
mvn exec:java -Dexec.mainClass=ui.JSidPlay2Main -Dexec.args="-E USBSID"
## THINKPAD UI
mvn exec:java -Dexec.mainClass=ui.JSidPlay2Main -Dexec.args="-E USBSID /home/loud/Development/c64/RETROCOLLECTION/SID-prgs/Hypersonic_Lovers_[8580].prg"
mvn exec:java -Dexec.mainClass=ui.JSidPlay2Main -Dexec.args="-E USBSID /home/loud/Development/c64/RETROCOLLECTION/SID-prgs/Supremacy.prg"
## THINKPAD CLI
mvn exec:java -Dexec.mainClass=sidplay.ConsolePlayer -Dexec.args="-E USBSID /home/loud/Development/c64/RETROCOLLECTION/SID-prgs/Supremacy.prg"
mvn exec:java -Dexec.mainClass=sidplay.ConsolePlayer -Dexec.args="-E USBSID /home/loud/Development/c64/RETROCOLLECTION/SID-prgs/Hypersonic_Lovers_[8580].prg"
```

# Jsidplay2 compile and test
```shell
mvn compile

# MAIN
mvn exec:java -Dexec.mainClass=sidplay.ConsolePlayer -Dexec.args="-E USBSID /mnt/loud/Code/Development/pi/USBSID-Pico-dev/private-repo/code-usbsid-related/configtool_webusb/SID/Commando.sid"

mvn exec:java -Dexec.mainClass=sidplay.ConsolePlayer -Dexec.args="-E USBSID /mnt/loud/Code/Development/pi/USBSID-Pico-dev/private-repo/code-usbsid-related/configtool_webusb/SID/digitunes/Afterburner.sid"

mvn exec:java -Dexec.mainClass=sidplay.ConsolePlayer -Dexec.args="-E USBSID /mnt/loud/Code/Development/pi/USBSID-Pico-dev/private-repo/code-usbsid-related/configtool_webusb/SID/digitunes/Coma_Light_13_tune_4.sid"

mvn -q exec:exec -Dexec.executable=java -Dexec.args="-cp %classpath sidplay.ConsolePlayer -E USBSID /mnt/loud/Code/Development/pi/USBSID-Pico-dev/private-repo/code-usbsid-related/configtool_webusb/SID/Commando.sid"

mvn -q exec:exec -Dexec.executable=java -Dexec.args="-cp %classpath sidplay.ConsolePlayer -E USBSID /mnt/loud/Code/Development/pi/USBSID-Pico-dev/private-repo/code-usbsid-related/configtool_webusb/SID/digitunes/Coma_Light_13_tune_4.sid"
```
# Debug
https://stackoverflow.com/questions/28752835/is-possible-have-byte-from-0-to-255-in-java
https://stackoverflow.com/questions/2935375/debugging-in-maven
```shell
# with suspend
MAVEN_OPTS="-Xdebug -Xnoagent -Djava.compiler=NONE -Xrunjdwp:transport=dt_socket,server=y,suspend=y,address=8000" ; mvnDebug exec:java -Dexec.mainClass=sidplay.ConsolePlayer -Dexec.args="-E USBSID /mnt/loud/Code/Development/pi/USBSID-Pico-dev/private-repo/code-usbsid-related/configtool_webusb/SID/Commando.sid"

mvn exec:exec -Dexec.executable="java" -Dexec.args="-classpath %classpath -Xdebug -Xrunjdwp:transport=dt_socket,server=y,suspend=y,address=8000 sidplay.ConsolePlayer -E USBSID /mnt/loud/Code/Development/pi/USBSID-Pico-dev/private-repo/code-usbsid-related/configtool_webusb/SID/Commando.sid"

# without suspend
MAVEN_OPTS="-Xdebug -Xnoagent -Djava.compiler=NONE -Xrunjdwp:transport=dt_socket,server=y,suspend=n,address=8000" ; mvnDebug exec:java -Dexec.mainClass=sidplay.ConsolePlayer -Dexec.args="-E USBSID /mnt/loud/Code/Development/pi/USBSID-Pico-dev/private-repo/code-usbsid-related/configtool_webusb/SID/Commando.sid"

mvn exec:exec -Dexec.executable="java" -Dexec.args="-classpath %classpath -Xdebug -Xrunjdwp:transport=dt_socket,server=y,suspend=n,address=1044 sidplay.ConsolePlayer"


```
