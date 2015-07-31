# Pipeline Implementation DOC

Pipeline consists of filters, observers.

In Serenity you are able to find and define several pipelines and choose which
should be use during modules loading.

Currently we've developed two pipelines:
1. CPU Estimator Pipeline for ResourcesEstimator.
2. CPU QoS Pipeline for QoSController.

## CPU Estimator Pipeline

![Image](https://github.com/mesosphere/serenity/blob/master/docs/images/cpu_estimator_pipeline.png)
    
## CPU QoS Pipeline

![Image](https://github.com/mesosphere/serenity/blob/master/docs/images/cpu_qos_pipeline.png)