import requests
import re

def search_Bilibili(keyword : str) -> list:

    headers = {
        'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36',
        'Referer': 'https://www.bilibili.com'
    }
    search_api = 'https://api.bilibili.com/x/web-interface/search/all/v2'
    params = {"keyword": keyword}

    r = requests.get(search_api, params=params, headers=headers)
    r.encoding = 'utf-8'

    content = r.json()["data"]["result"][-1]["data"]

    infos = []
    for item in content:
        title = (item["title"]).replace('<em class="keyword">', '').replace('</em>', '')

        duration = item["duration"]
        if len(duration) < 5:
            m,s = (item["duration"]).split(':')
            duration = f"{m.zfill(2)}:{s.zfill(2)}"

        infos.append([title, item["author"], duration, item["bvid"]])

    return infos


def download_audio(bvid : str, a_dir : str) -> None:

    headers = {
        'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36',
        'Referer': 'https://www.bilibili.com'
    }
    url = 'https://www.bilibili.com/video/' + bvid

    rr = requests.get(url, headers=headers)
    html = rr.text
    match = re.search(r'"audio"(?:.*?)"baseUrl":"(.*?)"](.*?)"audio/mp4"', html)
    audio_url = match.group(1)

    try:
        with requests.get(audio_url, headers=headers, stream=True, timeout=30) as r:
            r.raise_for_status()
            with open(f'{a_dir}/{bvid}.mp3', 'wb') as f:
                for chunk in r.iter_content(chunk_size=1024):
                    if chunk:  # 过滤掉保持连接的空块
                        f.write(chunk)
        print("python download success")
    except Exception as e:
        print(f"python download failed: {e}")



if __name__ == '__main__':
    download_audio("BV1npDNB6EKP")


