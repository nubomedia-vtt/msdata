  execute_process (COMMAND /usr/bin/kurento-module-creator -r /home/tteyli/uusnubomedia/msdata/src/server/interface -dr /usr/share/kurento/modules -o /home/tteyli/uusnubomedia/msdata/obj-x86_64-linux-gnu/src/server/)

  file (READ /home/tteyli/uusnubomedia/msdata/obj-x86_64-linux-gnu/src/server/msdata.kmd.json KMD_DATA)

  string (REGEX REPLACE "\n *" "" KMD_DATA ${KMD_DATA})
  string (REPLACE "\"" "\\\"" KMD_DATA ${KMD_DATA})
  string (REPLACE "\\n" "\\\\n" KMD_DATA ${KMD_DATA})
  set (KMD_DATA "\"${KMD_DATA}\"")

  file (WRITE /home/tteyli/uusnubomedia/msdata/obj-x86_64-linux-gnu/src/server/msdata.kmd.json ${KMD_DATA})
