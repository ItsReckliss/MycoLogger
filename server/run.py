"""Run the MycoLogger web server for local development or the BTT Pi."""

from __future__ import annotations

import uvicorn

from mycologger.settings import load_settings


if __name__ == "__main__":
    settings = load_settings()
    uvicorn.run(
        "mycologger.app:app",
        host=settings.host,
        port=settings.port,
        reload=False,
    )
