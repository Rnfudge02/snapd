# Predictor deny fixture

Disposable fork smoke-test PR. Do not merge into production.

Set repository variable PREDICTOR_SMOKE_MODE to deny and
PREDICTOR_SMOKE_CONTROLLER to completion. Apply the Auto rerun spread label
before marking the draft PR ready for review.

Expected: Tests fails at attempt 1, the bot report contains allowed=false,
and no automatic rerun occurs.

Fixture revision: 1