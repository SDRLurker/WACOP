# Watch and Calendar on PXA255 
임베디드 시스템 과목을 수강하며 했던 2인 프로젝트입니다.
## 프로젝트 하며 도움받은 정보
### 구조체 정렬, __attribute__ ((packed))
http://sdr1982.tistory.com/11

소스에서 비트맵 관련 구조체와 비트맵 파일의 구성 변수들이 서로 매칭이 되지 않는 문제가 있었습니다.

그래서 다음처럼 __attribute__ 속성을 주어 문제를 해결하였습니다.

<pre>
typedef struct tagBITMAPFILEHEADER
{
  unsigned shortbfType;// " BM"이라는 값을 저장함
  unsigned longbfSize;// 바이트단위로 전체파일 크기
  unsigned shortbfReserved1;// 예약된 변수
  unsigned shortbfReserved2;// 예약된 변수
  unsigned longbfOffBits;// 영상데이터 위치까지의 거리
} __attribute__ ((packed)) BITMAPFILEHEADER;
</pre>

### 라이브러리 정적 컴파일

다른 팀의 조원으로부터 얻은 이 정보를 통해 스레드(thread)를 사용할 수 있었습니다.

PXA255를 위한 크로스 컴파일시 arm-linux-gcc 에서 스레드를 사용하려면 
-lpthread 옵션 뿐 아니라 -static 옵션을 같이 사용하여야 컴파일이 되었습니다.

확실치는 않지만 PXA255에 설치된 리눅스에 스레드 관련 동적 라이브러리가 없어서 
정적 컴파일을 하여 스레드를 사용하도록 했던 걸로 추측합니다.

### 심볼테이블 제거

다른 팀의 조원에게 심볼 테이블을 제거하는 방법을 알려주었습니다. 즉, 정보를 서로 교환하였습니다.

<pre>
arm-linux-strip
</pre>

위의 명령을 활용하면 심볼 테이블이 제거가 되기 때문에 실행 파일의 용량이 줄어들 게 됩니다.

LINUX 명령의 strip과 동일한 명령입니다.
