import gdown
import os
import pathlib


def download(id):
    gdown.download(id=id, output="data.7z")
    os.system("7z x data.7z -odata")
    pathlib.Path("data.7z").unlink()


download("1NU1o0ld91i5jqUGbbBRokx932IXTdtnr")
download("1IPmVu2rtrLcDyaLzwml5aiAKLPEylMc8")
download("13mKjzk-eQfXoRuVQjbwaERkbucg7nvn3")
download("1o_MQHcFEg4_TL6FaCMxXd7sWPFQiAFvG")
download("1nf9f0hdYy5Nk2aUJjUObVK6nAa0I0Uwb")
download("1CdYNxVGoh8U4936U5lzpvJqWVRqr7wzg")
