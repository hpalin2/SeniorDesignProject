#include "views.hpp"
#include "util.hpp"
#include <sstream>
#include <iostream>

std::string render_dashboard(const std::vector<OperatingRoom>& rooms) {
    static const std::string css =
R"(body{margin:0;font-family:'Inter',system-ui,-apple-system,BlinkMacSystemFont,'Segoe UI',sans-serif;background:#0f172a;color:#e2e8f0;}a{color:#38bdf8;}main{max-width:1200px;margin:0 auto;padding:3rem 1.5rem;}header{text-align:center;margin-bottom:3rem;}header .logo{display:inline-flex;align-items:center;gap:0.75rem;margin-bottom:1rem;}header .logo-symbol{height:2.75rem;width:2.75rem;border-radius:0.9rem;background:#38bdf8;display:flex;align-items:center;justify-content:center;font-size:1.35rem;font-weight:700;color:#0f172a;}header .logo-text{font-size:1.5rem;font-weight:600;color:#e2e8f0;}h1{font-size:clamp(2.5rem,5vw,3.5rem);margin:0;color:#38bdf8;}p.subtitle{margin-top:0.5rem;color:#94a3b8;}section.cards{display:grid;grid-template-columns:repeat(auto-fit,minmax(260px,1fr));gap:1.5rem;}article.room-card{padding:1.5rem;border-radius:1.25rem;position:relative;overflow:hidden;box-shadow:0 15px 35px rgba(15,23,42,0.25);transition:transform 0.2s ease, box-shadow 0.2s ease;border:1px solid rgba(148,163,184,0.15);}article.room-card:hover{transform:translateY(-6px);box-shadow:0 20px 45px rgba(15,23,42,0.35);}article.room-card.room-card--ok{background:linear-gradient(135deg,rgba(22,163,74,0.95),rgba(21,128,61,0.9));color:#dcfce7;border-color:rgba(134,239,172,0.5);}article.room-card.room-card--warn{background:linear-gradient(135deg,rgba(234,179,8,0.95),rgba(202,138,4,0.9));color:#1f2937;border-color:rgba(234,179,8,0.55);}article.room-card .card-header{display:flex;align-items:center;justify-content:space-between;margin-bottom:0.5rem;}article.room-card h3{font-size:2rem;margin:0;font-weight:800;}article.room-card .meta{margin-top:0.25rem;font-weight:600;opacity:0.9;}article.room-card .time{font-size:0.85rem;opacity:0.8;}article.room-card .status{margin-top:1rem;font-size:1rem;font-weight:700;display:flex;align-items:center;gap:0.5rem;}article.room-card .status .icon{font-size:1.4rem;}article.room-card .status-icon{font-size:1.5rem;}footer{text-align:center;margin-top:3rem;color:#64748b;font-size:0.85rem;}footer span{font-weight:600;color:#38bdf8;}@media (prefers-color-scheme: light){body{background:#f8fafc;color:#0f172a;}article.room-card{box-shadow:0 10px 25px rgba(15,23,42,0.12);}})";

    std::ostringstream cards;
    for (const auto& room : rooms) {
        const bool suctionon = room.suction_on;
        const bool ok = (suctionon && room.occupancy) ||  (!suctionon && !room.occupancy);
        cards << "<article class='room-card " << (ok ? "room-card--ok" : "room-card--warn")
              << "' data-room-id='" << room.id << "'>";
        cards << "<div class='card-header'>";
        cards << "<h3>" << room.room_number << "</h3>";
        if (!ok) { cards << "<div class='status-icon' aria-hidden='true'>⚠️</div>"; }
        cards << "</div>";
        // meta shows the numeric occupancy (1 or 0) for quick debugging
        cards << "<p class='meta'>" << (room.occupancy ? "1" : "0") << "</p>";
        cards << "<p class='time'>" << room.schedule << "</p>";
        cards << "<div class='status'>";
        cards << "<span class='icon'>" << (suctionon ? "🟢" : "🔴") << "</span>";
        cards << "Suction: " << (suctionon ? "ON" : "OFF");
        cards << "</div></article>";
    }

    std::ostringstream page;
    page << "<!DOCTYPE html><html lang='en'><head><meta charset='utf-8'/>"
         << "<meta name='viewport' content='width=device-width,initial-scale=1'/>"
         << "<title>SuctionSense Dashboard</title>"
         << "<link rel='preconnect' href='https://fonts.googleapis.com'>"
         << "<link rel='preconnect' href='https://fonts.gstatic.com' crossorigin>"
         << "<link href='https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700;800&display=swap' rel='stylesheet'>"
         << "<style>" << css << "</style></head><body><main>";
    page << "<header><div class='logo'><div class='logo-symbol'>S</div>"
         << "<span class='logo-text'>SuctionSense</span></div>"
         << "<h1>Operating Room Suction Status</h1>"
         << "<p class='subtitle'>Real-time monitoring dashboard</p></header>";
    page << "<section class='cards'>" << cards.str() << "</section>";
    page << "<footer><p>Last updated: <span id='last-updated'>" << format_timestamp() << "</span></p></footer>";
    page << "</main>";

        page << R"(<script>
async function fetchData() {
  try {
    const res = await fetch('/api/rooms');
    const data = await res.json();
    const rooms = data.rooms;
    if (data.generatedAt) {
      document.getElementById('last-updated').textContent = data.generatedAt;
    }

    rooms.forEach(room => {
      console.log('room from API:', room);

      const card = document.querySelector(`[data-room-id='${room.id}']`);
      if (!card) return;

      // USE THE EXACT NAMES FROM rooms_to_json
      const suctionOn = room.suctionOn;     
      const occupancy = room.occupancy;     
      const schedule  = room.schedule;      

      const cardClass = (suctionOn && occupancy) || (!suctionOn && !occupancy)
        ? 'room-card--ok'
        : 'room-card--warn';

      card.classList.remove('room-card--ok','room-card--warn');
      card.classList.add(cardClass);

      const status = card.querySelector('.status');
      if (status) {
        status.innerHTML =
          `<span class='icon'>${suctionOn ? '🟢' : '🔴'}</span> ` +
          `Suction: ${suctionOn ? 'ON' : 'OFF'}`;
      }

      const meta = card.querySelector('.meta');
      if (meta) {
        meta.textContent = occupancy ? '1' : '0';
      }

      const timeEl = card.querySelector('.time');
      if (timeEl) {
        timeEl.textContent = schedule ? schedule : '';
      }
    });
  } catch (e) {
    console.error('update error', e);
  }
}
setInterval(fetchData, 5000);
</script>)";


    page << "</body></html>";
    return page.str();
}