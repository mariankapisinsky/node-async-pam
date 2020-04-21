const pam = require('bindings')('async_pam');

pam.authenticate('webapp', 'marian', item => {

  if (item.retval === -1) {
    console.log(item.prompt);

    setTimeout( () => {
      passwd = 'passwd';
      pam.registerResponse(item, passwd);
    }, 500);    
  } else {
    console.log(item.retval);
  };
});
