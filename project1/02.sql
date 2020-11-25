select name from (select type from (select count(*) as type_cnt0 from Pokemon as p0 group by p0.type order by type_cnt0 desc limit 2) as t1 inner join (select p1.type, count(*) as type_cnt1 from Pokemon as p1 group by p1.type) as t2 on t1.type_cnt0 = t2.type_cnt1) as t3 inner join Pokemon on t3.type = Pokemon.type order by name asc;