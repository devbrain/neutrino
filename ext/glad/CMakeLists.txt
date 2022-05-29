#I had same error message when I tried to install Python packages on my laptop with Windows 10 OS. I tried all methods recommended online and it still didn't work. I have been noticing for a while that Windows 10 automatically set proxy off causing Internet access problem sometime.
#
#Then I googled with keywords: 'windows 10 automatic proxy setting off'. Someone mentioned to modify regedit key value of [ProxyEnabled=1] under Computer\HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Internet Settings
#
#I opened the regedit and found it's [ProxyEnabled=1]. I compared HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Internet Settings with another workstation with relatively clean image. It's [ProxyEnabled=0] and no other extra entry for Proxy.
#
#Solution: set ProxyEnabled=0 and delete any other Proxy entries. It worked successfully! pip install package_name

fetchcontent_declare(
        glad
        GIT_REPOSITORY https://github.com/Dav1dde/glad.git
        GIT_TAG master
        UPDATE_COMMAND ""
)

fetchcontent_makeavailable(glad)


add_library(thirdparty::glad ALIAS glad)
set_thirdparty(glad)
set_thirdparty(glad-generate-files)