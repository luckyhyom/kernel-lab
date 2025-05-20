/*
 * 단순 문자 디바이스 드라이버: char_dev.c
 *
 * 이 모듈은 문자(character) 디바이스 드라이버의 기초를 구현합니다.
 * 사용자 공간에서 open, read, write, release 호출을 커널 모듈로 처리하며,
 * 장치 번호 등록, cdev 초기화/추가, 사용자-커널 메모리 복사를 예시로 보여줍니다.
 */
#include <linux/init.h>      /* __init, __exit: 초기화/종료 함수 매크로 */
#include <linux/module.h>    /* 모듈 인프라 제공: module_init, module_exit 등 */
#include <linux/fs.h>        /* 파일 시스템 및 디바이스 인터페이스: alloc_chrdev_region, struct file_operations 등 */
#include <linux/cdev.h>      /* 문자 디바이스 구조체 및 등록 함수: cdev_init, cdev_add, cdev_del */
#include <linux/uaccess.h>   /* 유저 공간 메모리 접근: copy_to_user, copy_from_user */

#define DEVICE_NAME "mychardev"  /* /proc/devices에 표시될 디바이스 이름 */
#define BUF_LEN     100           /* 내부 버퍼 크기(바이트) */

/*
 * dev_t: 디바이스 번호(major/minor)를 하나의 값으로 표현하는 형
 *    MAJOR(dev_num), MINOR(dev_num)로 구분
 */
static dev_t dev_num;

/*
 * struct cdev: 커널 내부에서 문자 디바이스를 나타내는 객체
 *    cdev_init로 초기화하고 cdev_add로 커널에 등록
 */
static struct cdev my_cdev;

/*
 * msg: 사용자로부터 write된 데이터를 저장하는 버퍼
 * msg_size: 실제 msg에 저장된 바이트 수(size_t, 부호 없는 정수)
 */
static char msg[BUF_LEN];
static size_t msg_size;

/*
 * my_open: /dev/mychardev를 open()할 때 호출
 * @inode: 디바이스에 해당하는 inode 구조체 포인터
 * @file: 파일 오브젝트 포인터 (파일 오프셋, 플래그 등 보관)
 */
static int my_open(struct inode *inode, struct file *file) {
    printk(KERN_INFO "%s: open\n", DEVICE_NAME);
    /* 성공 시 0 반환, 비-제로 시 open() 실패 */
    return 0;
}

/*
 * my_release: close() 호출 시 실행
 */
static int my_release(struct inode *inode, struct file *file) {
    printk(KERN_INFO "%s: release\n", DEVICE_NAME);
    return 0;
}

/*
 * my_read: 사용자 공간으로 데이터 복사
 * @file:  파일 오브젝트
 * @buf:   사용자 버퍼(사용자 주소 공간)
 * @count: 요청된 바이트 수(size_t, 부호 없는 정수)
 * @ppos:  파일 오프셋(loff_t, 64비트 정수)
 * 반환값: 복사된 바이트 수(ssize_t, 부호 있는 정수) 또는 오류 코드
 */
static ssize_t my_read(struct file *file, char __user *buf, size_t count, loff_t *ppos) {
    size_t avail = msg_size - *ppos; /* 남은 데이터 양 */
    ssize_t to_copy;

    if (avail == 0)
        return 0; /* 읽을 데이터 없음 => EOF */

    /* 요청된 크기와 남은 데이터 중 작은 값을 선택 */
    to_copy = (count < avail) ? count : avail;

    /* 커널 버퍼(msg + *ppos)에서 사용자 공간(buf)으로 복사 */
    if (copy_to_user(buf, msg + *ppos, to_copy))
        return -EFAULT; /* 사용자 메모리 접근 실패 */

    *ppos += to_copy; /* 오프셋 이동 */
    printk(KERN_INFO "%s: read %zd bytes\n", DEVICE_NAME, to_copy);
    return to_copy;
}

/*
 * my_write: 사용자 공간에서 커널 버퍼로 데이터 복사
 * @file:  파일 오브젝트
 * @buf:   사용자 버퍼
 * @count: 전달된 바이트 수
 * @ppos:  사용되지 않음
 * 반환값: 복사된 바이트 수 또는 오류 코드
 */
static ssize_t my_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos) {
    size_t to_copy = (count < BUF_LEN - 1) ? count : (BUF_LEN - 1);

    /* 사용자 메모리(buf)에서 커널 메모리(msg)로 복사 */
    if (copy_from_user(msg, buf, to_copy))
        return -EFAULT;

    msg[to_copy] = '\0'; /* 문자열 종료자 추가 */
    msg_size = to_copy;    /* 저장된 데이터 크기 갱신 */
    printk(KERN_INFO "%s: write %zu bytes: %s\n", DEVICE_NAME, to_copy, msg);
    return to_copy;
}

/*
 * 파일 연산 콜백을 위한 함수 테이블
 * open, release, read, write 호출 시 연결된 함수가 실행됨
 */
static const struct file_operations fops = {
    .owner   = THIS_MODULE, /* 모듈 참조 카운트를 유지 */
    .open    = my_open,
    .release = my_release,
    .read    = my_read,
    .write   = my_write,
};

/*
 * char_init: 모듈 로드 시 호출되는 초기화 함수
 * - alloc_chrdev_region: major/minor 번호 할당
 * - cdev_init, cdev_add: 문자 디바이스 등록
 */
static int __init char_init(void) {
    int ret;

    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    /* 실패 시 음수 반환, 성공 시 0 */
    if (ret) {
        printk(KERN_ERR "%s: alloc_chrdev_region failed\n", DEVICE_NAME);
        return ret;
    }

    cdev_init(&my_cdev, &fops); /* cdev 구조체에 콜백 연결 */
    my_cdev.owner = THIS_MODULE;

    ret = cdev_add(&my_cdev, dev_num, 1);
    if (ret) {
        unregister_chrdev_region(dev_num, 1); /* 할당 해제 */
        printk(KERN_ERR "%s: cdev_add failed\n", DEVICE_NAME);
        return ret;
    }

    printk(KERN_INFO "%s: registered major=%d minor=%d\n",
           DEVICE_NAME, MAJOR(dev_num), MINOR(dev_num));
    return 0;
}

/*
 * char_exit: 모듈 언로드 시 호출되는 종료 함수
 * - cdev_del: cdev 제거
 * - unregister_chrdev_region: 번호 해제
 */
static void __exit char_exit(void) {
    cdev_del(&my_cdev);
    unregister_chrdev_region(dev_num, 1);
    printk(KERN_INFO "%s: unregistered\n", DEVICE_NAME);
}

/* 모듈 로드/언로드 진입점 지정 */
module_init(char_init);
module_exit(char_exit);

/* 모듈 정보 */
MODULE_LICENSE("GPL");          /* GPL 라이선스 (필수) */
MODULE_AUTHOR("Your Name");    /* 작성자 정보 */
MODULE_DESCRIPTION("Simple Character Device Driver"); /* 설명 */
