cmake_minimum_required(VERSION 2.8)

file(GLOB circle_src "*.cpp")
SET(circle_src ${circle_src} PARENT_SCOPE)

add_subdirectory(util)

SET(autowiz_src   ${autowiz_src}   PARENT_SCOPE)
SET(listrent_src  ${listrent_src}  PARENT_SCOPE)
SET(play2to3_src  ${play2to3_src}  PARENT_SCOPE)
SET(shopconv_src  ${shopconv_src}  PARENT_SCOPE)
SET(sign_src      ${sign_src}      PARENT_SCOPE)
SET(wld2html_src  ${wld2html_src}  PARENT_SCOPE)
SET(delobjs_src   ${delobjs_src}   PARENT_SCOPE)
SET(mudpasswd_src ${mudpasswd_src} PARENT_SCOPE)
SET(purgeplay_src ${purgeplay_src} PARENT_SCOPE)
SET(showplay_src  ${showplay_src}  PARENT_SCOPE)
SET(split_src     ${split_src}     PARENT_SCOPE)
