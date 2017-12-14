
/**
 * @api {post} /calc/add Addtion
 * @apiVersion 0.1.0
 * @apiName add
 * @apiGroup calc
 *
 * @apiDescription description
 *
 * @apiParam {Number} a
 * @apiParam {Number} b
 *
 * @apiExample Example usage:
 * curl -i http://localhost/calc/add
 *      -d { "a": 1, "b":2}
 *
 * @apiSuccess {Number} result of a + b
 */
 
 /**
 * @api {post} /calc/square/:num Square
 * @apiVersion 0.1.0
 * @apiName power
 * @apiGroup calc
 *
 * @apiDescription description
 *
 * @apiParam {Number} num
 *
 * @apiExample Example usage:
 * curl -i http://localhost/calc/square/10
 *
 * @apiSuccess {Number} result of 10^2
 * 
 */
 
 
 /**
 * @api {get} /calc/timer/:timeout timeout response
 * @apiVersion 0.1.0
 * @apiName timer
 * @apiGroup calc
 *
 * @apiDescription description
 *
 * @apiParam {Number} timeout in ms
 *
 * @apiExample Example usage:
 * curl -i http://localhost/calc/timer/1000
 *
 * @apiSuccess {Number} response after 1000ms
 */