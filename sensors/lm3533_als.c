#define LOG_TAG "DASH - light"

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include <linux/input.h>
#include <errno.h>
#include "sensors_log.h"
#include "sensors_list.h"
#include "sensors_fifo.h"
#include "sensors_worker.h"
#include "sensor_util.h"
#include "sensors_id.h"

#define ALS_ENABLE "/sys/devices/i2c-0/0-0036/als_enable"
#define ALS_RESULT "/sys/devices/i2c-0/0-0036/als_result"

static struct sensor_desc light_sensor;

struct sensor_desc {
	struct sensors_worker_t worker;
	struct sensor_t sensor;
	struct sensor_api_t api;
	int fd;
};

static int write_int(char const *path, int value)
{
	int fd;
	static int already_warned;

	already_warned = 0;

	ALOGV("write_int: path %s, value %d", path, value);
	fd = open(path, O_RDWR);

	if (fd >= 0) {
		char buffer[20];
		int bytes = sprintf(buffer, "%d\n", value);
		int amt = write(fd, buffer, bytes);
		close(fd);
		return amt == -1 ? -errno : 0;
	} else {
		if (already_warned == 0) {
			ALOGE("write_int failed to open %s\n", path);
			already_warned = 1;
		}
		return -errno;
	}
}

static void *light_poll(void *arg)
{
	struct sensor_desc *d = container_of(arg, struct sensor_desc, worker);
	sensors_event_t data;
	char buf[20];
	ssize_t n;
	int lux;

	memset(&data, 0, sizeof(data));

	n = pread(d->fd, buf, sizeof(buf), 0);

	/*convert to lux value*/
	lux = atof(buf)*6;

	data.light = lux;
	data.version = light_sensor.sensor.version;
	data.sensor = light_sensor.sensor.handle;
	data.type = light_sensor.sensor.type;
	data.timestamp = get_current_nano_time();
	sensors_fifo_put(&data);

	return NULL;
}

static int light_init(struct sensor_api_t *s)
{
	struct sensor_desc *d = container_of(s, struct sensor_desc, api);

	sensors_worker_init(&d->worker, light_poll, &d->worker);

	return 0;
}

static int light_activate(struct sensor_api_t *s, int enable)
{
    int fd_enable;
	int fd;
	struct sensor_desc *d = container_of(s, struct sensor_desc, api);

	if (enable) {
        write_int(ALS_ENABLE, 1);
		fd = open(ALS_RESULT, O_RDONLY);
		d->fd = fd;
		d->worker.resume(&d->worker);
	} else {
		d->worker.suspend(&d->worker);
		close(d->fd);
		d->fd = -1;
        write_int(ALS_ENABLE, 0);
	}

	return 0;
}

static int light_set_delay(struct sensor_api_t *s, int64_t ns)
{
	struct sensor_desc *d = container_of(s, struct sensor_desc, api);

	d->worker.set_delay(&d->worker, ns);

	return 0;
}

static void light_close(struct sensor_api_t *s)
{
	struct sensor_desc *d = container_of(s, struct sensor_desc, api);

	d->worker.destroy(&d->worker);
}

static struct sensor_desc light_sensor = {
	.sensor = {
		.name = "LM3533 based light sensor",
		.vendor = "The CyanogenMod Project",
		.version = sizeof(sensors_event_t),
		.handle = SENSOR_LIGHTSENSOR_HANDLE,
		.type = SENSOR_TYPE_LIGHT,
		.maxRange = 1530,
		.resolution = 1.0,
		.power = 1
	},
	.api = {
		.init = light_init,
		.activate = light_activate,
		.set_delay = light_set_delay,
		.close = light_close
	},
	.fd = -1,
};

list_constructor(light_init_driver);
void light_init_driver()
{
	(void)sensors_list_register(&light_sensor.sensor, &light_sensor.api);
}
